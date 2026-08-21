-- =====================================
-- 1. 创建数据库
-- =====================================
CREATE DATABASE IF NOT EXISTS bilibili_anime
  CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE bilibili_anime;

-- =====================================
-- 2. 表定义
-- =====================================

-- 番剧主表
CREATE TABLE IF NOT EXISTS anime (
  anime_id INT AUTO_INCREMENT PRIMARY KEY,
  title VARCHAR(255) NOT NULL,
  year YEAR,
  region VARCHAR(50),
  views BIGINT UNSIGNED NOT NULL DEFAULT 0,
  rating DECIMAL(3,1) DEFAULT NULL,
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY ux_anime_title_year (title, year)
) ENGINE=InnoDB;

-- 制作单位
CREATE TABLE IF NOT EXISTS studio (
  name VARCHAR(100) PRIMARY KEY,
  flagship_work VARCHAR(255),
  created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- 标签
CREATE TABLE IF NOT EXISTS tag (
  name VARCHAR(50) PRIMARY KEY,
  usage_count INT UNSIGNED NOT NULL DEFAULT 0
) ENGINE=InnoDB;

-- 导演
CREATE TABLE IF NOT EXISTS director (
  director_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(100) NOT NULL,
  nationality VARCHAR(50),
  flagship_work VARCHAR(255)
) ENGINE=InnoDB;

-- 声优
CREATE TABLE IF NOT EXISTS seiyuu (
  seiyuu_id INT AUTO_INCREMENT PRIMARY KEY,
  name VARCHAR(100) NOT NULL,
  nationality VARCHAR(50),
  flagship_work VARCHAR(255)
) ENGINE=InnoDB;

-- 原作
CREATE TABLE IF NOT EXISTS original_work (
  title VARCHAR(255) NOT NULL,
  author VARCHAR(100) NOT NULL,
  region VARCHAR(50),
  PRIMARY KEY (title, author)
) ENGINE=InnoDB;

-- 改编动画子表
CREATE TABLE IF NOT EXISTS adapted_anime (
  anime_id INT PRIMARY KEY,
  original_title VARCHAR(255) NOT NULL,
  original_author VARCHAR(100) NOT NULL,
  FOREIGN KEY (anime_id) REFERENCES anime(anime_id) ON DELETE CASCADE,
  FOREIGN KEY (original_title, original_author)
    REFERENCES original_work(title, author)
    ON DELETE RESTRICT
) ENGINE=InnoDB;

-- 关系表：production
CREATE TABLE IF NOT EXISTS production (
  anime_id INT NOT NULL,
  studio_name VARCHAR(100) NOT NULL,
  PRIMARY KEY (anime_id, studio_name),
  FOREIGN KEY (anime_id)   REFERENCES anime(anime_id)   ON DELETE CASCADE,
  FOREIGN KEY (studio_name) REFERENCES studio(name)     ON DELETE RESTRICT
) ENGINE=InnoDB;

-- 关系表：adaptation
CREATE TABLE IF NOT EXISTS adaptation (
  anime_id INT NOT NULL,
  type VARCHAR(50) NOT NULL,
  original_title VARCHAR(255) NOT NULL,
  original_author VARCHAR(100) NOT NULL,
  PRIMARY KEY (anime_id, original_title, original_author),
  FOREIGN KEY (anime_id) REFERENCES adapted_anime(anime_id) ON DELETE CASCADE,
  FOREIGN KEY (original_title, original_author)
    REFERENCES original_work(title, author)
    ON DELETE RESTRICT
) ENGINE=InnoDB;

-- 关系表：directorship
CREATE TABLE IF NOT EXISTS directorship (
  director_id INT NOT NULL,
  anime_id INT NOT NULL,
  PRIMARY KEY (director_id, anime_id),
  FOREIGN KEY (director_id) REFERENCES director(director_id) ON DELETE RESTRICT,
  FOREIGN KEY (anime_id)   REFERENCES anime(anime_id)     ON DELETE CASCADE
) ENGINE=InnoDB;

-- 关系表：anime_tag
CREATE TABLE IF NOT EXISTS anime_tag (
  tag_name VARCHAR(50) NOT NULL,
  anime_id INT NOT NULL,
  PRIMARY KEY (tag_name, anime_id),
  FOREIGN KEY (tag_name) REFERENCES tag(name)       ON DELETE CASCADE,
  FOREIGN KEY (anime_id)  REFERENCES anime(anime_id) ON DELETE CASCADE
) ENGINE=InnoDB;

-- 关系表：casting
CREATE TABLE IF NOT EXISTS casting (
  seiyuu_id INT NOT NULL,
  anime_id INT NOT NULL,
  PRIMARY KEY (seiyuu_id, anime_id),
  FOREIGN KEY (seiyuu_id) REFERENCES seiyuu(seiyuu_id) ON DELETE RESTRICT,
  FOREIGN KEY (anime_id)   REFERENCES anime(anime_id)   ON DELETE CASCADE
) ENGINE=InnoDB;

-- =====================================
-- 审计日志表：记录所有表的增删改操作
-- =====================================
CREATE TABLE IF NOT EXISTS operation_audit (
  audit_id     INT AUTO_INCREMENT PRIMARY KEY,
  table_name   VARCHAR(64)     NOT NULL,              -- 操作的表名
  operation    ENUM('INSERT','UPDATE','DELETE') NOT NULL,  -- 操作类型
  record_id    VARCHAR(255)    NOT NULL,              -- 被操作记录的主键或组合键
  old_data     JSON            NULL,                  -- UPDATE/DELETE 前的数据（JSON）
  new_data     JSON            NULL,                  -- INSERT/UPDATE 后的数据（JSON）
  changed_at   TIMESTAMP       NOT NULL DEFAULT CURRENT_TIMESTAMP -- 操作时间
) ENGINE=InnoDB
  DEFAULT CHARSET = utf8mb4
  COLLATE = utf8mb4_unicode_ci;


-- =====================================
-- 3. 视图定义
-- =====================================

-- 3.1 播放量 Top 10
CREATE OR REPLACE VIEW view_top10_by_views AS
SELECT anime_id, title, views
FROM anime
ORDER BY views DESC
LIMIT 10;

-- 3.2 评分 Top 10
CREATE OR REPLACE VIEW view_top10_by_rating AS
SELECT anime_id, title, rating
FROM anime
WHERE rating IS NOT NULL
ORDER BY rating DESC
LIMIT 10;

-- 3.3 制作单位番剧统计
CREATE OR REPLACE VIEW view_studio_counts AS
SELECT p.studio_name, COUNT(*) AS anime_count
FROM production p
GROUP BY p.studio_name;

-- =====================================
-- 4. 触发器定义
-- =====================================

--  在 anime_tag 插入/删除时维护 tag.usage_count
DELIMITER $$
CREATE TRIGGER trg_tag_increment AFTER INSERT ON anime_tag
FOR EACH ROW
BEGIN
  UPDATE tag
  SET usage_count = usage_count + 1
  WHERE name = NEW.tag_name;
END$$

CREATE TRIGGER trg_tag_decrement AFTER DELETE ON anime_tag
FOR EACH ROW
BEGIN
  UPDATE tag
  SET usage_count = GREATEST(usage_count - 1, 0)
  WHERE name = OLD.tag_name;
END$$
DELIMITER ;

-- 在 anime.views 更新时写入日志表（用于历史分析）
CREATE TABLE IF NOT EXISTS anime_views_log (
  log_id INT AUTO_INCREMENT PRIMARY KEY,
  anime_id INT NOT NULL,
  old_views BIGINT,
  new_views BIGINT,
  changed_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (anime_id) REFERENCES anime(anime_id) ON DELETE CASCADE
) ENGINE=InnoDB;

DELIMITER $$
CREATE TRIGGER trg_views_change
BEFORE UPDATE ON anime
FOR EACH ROW
BEGIN
  IF NEW.views <> OLD.views THEN
    INSERT INTO anime_views_log(anime_id, old_views, new_views)
    VALUES (OLD.anime_id, OLD.views, NEW.views);
  END IF;
END$$
DELIMITER ;


DELIMITER $$

-- ========== 1. ANIME 表校验 & 审计 ==========
-- 插入前校验
CREATE TRIGGER chk_anime_before_ins
BEFORE INSERT ON anime
FOR EACH ROW
BEGIN
  IF NEW.views < 0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime.views 必须 ≥ 0';
  END IF;
  IF NEW.rating IS NOT NULL AND (NEW.rating<0 OR NEW.rating>10) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime.rating 必须在 0–10 之间';
  END IF;
  IF NEW.year < 1900 OR NEW.year > YEAR(CURDATE()) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime.year 超出有效范围';
  END IF;
END$$

-- 更新前校验
CREATE TRIGGER chk_anime_before_upd
BEFORE UPDATE ON anime
FOR EACH ROW
BEGIN
  IF NEW.views < 0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime.views 必须 ≥ 0';
  END IF;
  IF NEW.rating IS NOT NULL AND (NEW.rating<0 OR NEW.rating>10) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime.rating 必须在 0–10 之间';
  END IF;
  IF NEW.year < 1900 OR NEW.year > YEAR(CURDATE()) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime.year 超出有效范围';
  END IF;
END$$

-- 插入后审计
CREATE TRIGGER audit_anime_after_ins
AFTER INSERT ON anime
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES(
    'anime','INSERT',CAST(NEW.anime_id AS CHAR),
    JSON_OBJECT('title',NEW.title,'year',NEW.year,'region',NEW.region,
                'views',NEW.views,'rating',NEW.rating)
  );
END$$

-- 更新后审计
CREATE TRIGGER audit_anime_after_upd
AFTER UPDATE ON anime
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data,new_data)
  VALUES(
    'anime','UPDATE',CAST(OLD.anime_id AS CHAR),
    JSON_OBJECT('title',OLD.title,'year',OLD.year,'region',OLD.region,
                'views',OLD.views,'rating',OLD.rating),
    JSON_OBJECT('title',NEW.title,'year',NEW.year,'region',NEW.region,
                'views',NEW.views,'rating',NEW.rating)
  );
END$$

-- 删除后审计
CREATE TRIGGER audit_anime_after_del
AFTER DELETE ON anime
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES(
    'anime','DELETE',CAST(OLD.anime_id AS CHAR),
    JSON_OBJECT('title',OLD.title,'year',OLD.year,'region',OLD.region,
                'views',OLD.views,'rating',OLD.rating)
  );
END$$


-- ========== 2. STUDIO 表防删 & 审计 ==========
CREATE TRIGGER prevent_studio_before_del
BEFORE DELETE ON studio
FOR EACH ROW
BEGIN
  DECLARE cnt INT;
  SELECT COUNT(*) INTO cnt FROM production WHERE studio_name=OLD.name;
  IF cnt>0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='studio 存在关联番剧，不可删除';
  END IF;
END$$

CREATE TRIGGER audit_studio_after_ins
AFTER INSERT ON studio
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES('studio','INSERT',NEW.name,
         JSON_OBJECT('flagship_work',NEW.flagship_work));
END$$

CREATE TRIGGER audit_studio_after_upd
AFTER UPDATE ON studio
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data,new_data)
  VALUES('studio','UPDATE',OLD.name,
         JSON_OBJECT('flagship_work',OLD.flagship_work),
         JSON_OBJECT('flagship_work',NEW.flagship_work));
END$$

CREATE TRIGGER audit_studio_after_del
AFTER DELETE ON studio
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES('studio','DELETE',OLD.name,
         JSON_OBJECT('flagship_work',OLD.flagship_work));
END$$


-- ========== 3. TAG 表审计==========
CREATE TRIGGER audit_tag_after_ins
AFTER INSERT ON tag
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES('tag','INSERT',NEW.name,
         JSON_OBJECT('usage_count',NEW.usage_count));
END$$

CREATE TRIGGER audit_tag_after_upd
AFTER UPDATE ON tag
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data,new_data)
  VALUES('tag','UPDATE',OLD.name,
         JSON_OBJECT('usage_count',OLD.usage_count),
         JSON_OBJECT('usage_count',NEW.usage_count));
END$$

CREATE TRIGGER audit_tag_after_del
AFTER DELETE ON tag
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES('tag','DELETE',OLD.name,
         JSON_OBJECT('usage_count',OLD.usage_count));
END$$


-- ========== 4. DIRECTOR 表防删 & 审计 ==========
CREATE TRIGGER prevent_director_before_del
BEFORE DELETE ON director
FOR EACH ROW
BEGIN
  DECLARE cnt INT;
  SELECT COUNT(*) INTO cnt FROM directorship WHERE director_id=OLD.director_id;
  IF cnt>0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='director 存在关联番剧，不可删除';
  END IF;
END$$

CREATE TRIGGER audit_director_after_ins
AFTER INSERT ON director
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES('director','INSERT',CAST(NEW.director_id AS CHAR),
         JSON_OBJECT('name',NEW.name,'nationality',NEW.nationality));
END$$

CREATE TRIGGER audit_director_after_upd
AFTER UPDATE ON director
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data,new_data)
  VALUES('director','UPDATE',CAST(OLD.director_id AS CHAR),
         JSON_OBJECT('name',OLD.name,'nationality',OLD.nationality),
         JSON_OBJECT('name',NEW.name,'nationality',NEW.nationality));
END$$

CREATE TRIGGER audit_director_after_del
AFTER DELETE ON director
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES('director','DELETE',CAST(OLD.director_id AS CHAR),
         JSON_OBJECT('name',OLD.name,'nationality',OLD.nationality));
END$$


-- ========== 5. SEIYUU 表防删 & 审计 ==========
CREATE TRIGGER prevent_seiyuu_before_del
BEFORE DELETE ON seiyuu
FOR EACH ROW
BEGIN
  DECLARE cnt INT;
  SELECT COUNT(*) INTO cnt FROM casting WHERE seiyuu_id=OLD.seiyuu_id;
  IF cnt>0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='seiyuu 存在关联番剧，不可删除';
  END IF;
END$$

CREATE TRIGGER audit_seiyuu_after_ins
AFTER INSERT ON seiyuu
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES('seiyuu','INSERT',CAST(NEW.seiyuu_id AS CHAR),
         JSON_OBJECT('name',NEW.name,'nationality',NEW.nationality));
END$$

CREATE TRIGGER audit_seiyuu_after_upd
AFTER UPDATE ON seiyuu
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data,new_data)
  VALUES('seiyuu','UPDATE',CAST(OLD.seiyuu_id AS CHAR),
         JSON_OBJECT('name',OLD.name,'nationality',OLD.nationality),
         JSON_OBJECT('name',NEW.name,'nationality',NEW.nationality));
END$$

CREATE TRIGGER audit_seiyuu_after_del
AFTER DELETE ON seiyuu
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES('seiyuu','DELETE',CAST(OLD.seiyuu_id AS CHAR),
         JSON_OBJECT('name',OLD.name,'nationality',OLD.nationality));
END$$


-- ========== 6. ORIGINAL_WORK 表防删 & 审计 ==========
CREATE TRIGGER prevent_original_before_del
BEFORE DELETE ON original_work
FOR EACH ROW
BEGIN
  DECLARE cnt INT;
  SELECT COUNT(*) INTO cnt FROM adaptation WHERE original_title=OLD.title AND original_author=OLD.author;
  IF cnt>0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='original_work 已被改编，不可删除';
  END IF;
END$$

CREATE TRIGGER audit_original_after_ins
AFTER INSERT ON original_work
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES('original_work','INSERT',CONCAT(NEW.title,'|',NEW.author),
         JSON_OBJECT('region',NEW.region));
END$$

CREATE TRIGGER audit_original_after_upd
AFTER UPDATE ON original_work
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data,new_data)
  VALUES('original_work','UPDATE',CONCAT(OLD.title,'|',OLD.author),
         JSON_OBJECT('region',OLD.region),
         JSON_OBJECT('region',NEW.region));
END$$

CREATE TRIGGER audit_original_after_del
AFTER DELETE ON original_work
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES('original_work','DELETE',CONCAT(OLD.title,'|',OLD.author),
         JSON_OBJECT('region',OLD.region));
END$$


-- ========== 7. 关系表：production, anime_tag, directorship, casting, adapted_anime, adaptation ==========
-- 7.1 production
CREATE TRIGGER chk_production_before_ins
BEFORE INSERT ON production
FOR EACH ROW
BEGIN
  IF NOT EXISTS (SELECT 1 FROM anime WHERE anime_id=NEW.anime_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='production: anime 不存在';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM studio WHERE name=NEW.studio_name) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='production: studio 不存在';
  END IF;
END$$

CREATE TRIGGER audit_production_after_ins
AFTER INSERT ON production
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES('production','INSERT',CONCAT(NEW.anime_id,'|',NEW.studio_name),
         JSON_OBJECT('anime_id',NEW.anime_id,'studio_name',NEW.studio_name));
END$$

CREATE TRIGGER audit_production_after_del
AFTER DELETE ON production
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES('production','DELETE',CONCAT(OLD.anime_id,'|',OLD.studio_name),
         JSON_OBJECT('anime_id',OLD.anime_id,'studio_name',OLD.studio_name));
END$$

-- ========== 7.2 anime_tag ==========
CREATE TRIGGER chk_anime_tag_before_ins
BEFORE INSERT ON anime_tag
FOR EACH ROW
BEGIN
  IF NOT EXISTS (SELECT 1 FROM anime WHERE anime_id = NEW.anime_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime_tag 插入失败：anime 不存在';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM tag WHERE name = NEW.tag_name) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='anime_tag 插入失败：tag 不存在';
  END IF;
END$$

CREATE TRIGGER audit_anime_tag_after_ins
AFTER INSERT ON anime_tag
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES(
    'anime_tag','INSERT',CONCAT(NEW.anime_id,'|',NEW.tag_name),
    JSON_OBJECT('anime_id',NEW.anime_id,'tag_name',NEW.tag_name)
  );
END$$

CREATE TRIGGER audit_anime_tag_after_del
AFTER DELETE ON anime_tag
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES(
    'anime_tag','DELETE',CONCAT(OLD.anime_id,'|',OLD.tag_name),
    JSON_OBJECT('anime_id',OLD.anime_id,'tag_name',OLD.tag_name)
  );
END$$

-- ========== 7.3 directorship ==========
CREATE TRIGGER chk_directorship_before_ins
BEFORE INSERT ON directorship
FOR EACH ROW
BEGIN
  IF NOT EXISTS (SELECT 1 FROM anime WHERE anime_id = NEW.anime_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='directorship 插入失败：anime 不存在';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM director WHERE director_id = NEW.director_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='directorship 插入失败：director 不存在';
  END IF;
END$$

CREATE TRIGGER audit_directorship_after_ins
AFTER INSERT ON directorship
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES(
    'directorship','INSERT',CONCAT(NEW.director_id,'|',NEW.anime_id),
    JSON_OBJECT('director_id',NEW.director_id,'anime_id',NEW.anime_id)
  );
END$$

CREATE TRIGGER audit_directorship_after_del
AFTER DELETE ON directorship
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES(
    'directorship','DELETE',CONCAT(OLD.director_id,'|',OLD.anime_id),
    JSON_OBJECT('director_id',OLD.director_id,'anime_id',OLD.anime_id)
  );
END$$

-- ========== 7.4 casting ==========
CREATE TRIGGER chk_casting_before_ins
BEFORE INSERT ON casting
FOR EACH ROW
BEGIN
  IF NOT EXISTS (SELECT 1 FROM anime WHERE anime_id = NEW.anime_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='casting 插入失败：anime 不存在';
  END IF;
  IF NOT EXISTS (SELECT 1 FROM seiyuu WHERE seiyuu_id = NEW.seiyuu_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='casting 插入失败：seiyuu 不存在';
  END IF;
END$$

CREATE TRIGGER audit_casting_after_ins
AFTER INSERT ON casting
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES(
    'casting','INSERT',CONCAT(NEW.seiyuu_id,'|',NEW.anime_id),
    JSON_OBJECT('seiyuu_id',NEW.seiyuu_id,'anime_id',NEW.anime_id)
  );
END$$

CREATE TRIGGER audit_casting_after_del
AFTER DELETE ON casting
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES(
    'casting','DELETE',CONCAT(OLD.seiyuu_id,'|',OLD.anime_id),
    JSON_OBJECT('seiyuu_id',OLD.seiyuu_id,'anime_id',OLD.anime_id)
  );
END$$

-- ========== 7.5 adapted_anime ==========
CREATE TRIGGER chk_adapted_anime_before_ins
BEFORE INSERT ON adapted_anime
FOR EACH ROW
BEGIN
  IF NOT EXISTS (SELECT 1 FROM anime WHERE anime_id = NEW.anime_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='adapted_anime 插入失败：anime 不存在';
  END IF;
  IF NOT EXISTS (
    SELECT 1 FROM original_work 
    WHERE title=NEW.original_title AND author=NEW.original_author
  ) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='adapted_anime 插入失败：original_work 不存在';
  END IF;
END$$

CREATE TRIGGER prevent_adapted_anime_before_del
BEFORE DELETE ON adapted_anime
FOR EACH ROW
BEGIN
  DECLARE cnt INT;
  SELECT COUNT(*) INTO cnt FROM adaptation WHERE anime_id=OLD.anime_id;
  IF cnt>0 THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='adapted_anime 下有改编记录，不可删除';
  END IF;
END$$

CREATE TRIGGER audit_adapted_anime_after_ins
AFTER INSERT ON adapted_anime
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES(
    'adapted_anime','INSERT',CAST(NEW.anime_id AS CHAR),
    JSON_OBJECT('original_title',NEW.original_title,'original_author',NEW.original_author)
  );
END$$

CREATE TRIGGER audit_adapted_anime_after_del
AFTER DELETE ON adapted_anime
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES(
    'adapted_anime','DELETE',CAST(OLD.anime_id AS CHAR),
    JSON_OBJECT('original_title',OLD.original_title,'original_author',OLD.original_author)
  );
END$$

-- ========== 7.6 adaptation ==========
CREATE TRIGGER chk_adaptation_before_ins
BEFORE INSERT ON adaptation
FOR EACH ROW
BEGIN
  IF NOT EXISTS (SELECT 1 FROM adapted_anime WHERE anime_id = NEW.anime_id) THEN
    SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='adaptation 插入失败：adapted_anime 不存在';
  END IF;
END$$

CREATE TRIGGER audit_adaptation_after_ins
AFTER INSERT ON adaptation
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,new_data)
  VALUES(
    'adaptation','INSERT',
    CONCAT(NEW.anime_id,'|',NEW.original_title,'|',NEW.original_author),
    JSON_OBJECT(
      'anime_id',NEW.anime_id,'type',NEW.type,
      'original_title',NEW.original_title,'original_author',NEW.original_author
    )
  );
END$$

CREATE TRIGGER audit_adaptation_after_del
AFTER DELETE ON adaptation
FOR EACH ROW
BEGIN
  INSERT INTO operation_audit(table_name,operation,record_id,old_data)
  VALUES(
    'adaptation','DELETE',
    CONCAT(OLD.anime_id,'|',OLD.original_title,'|',OLD.original_author),
    JSON_OBJECT(
      'anime_id',OLD.anime_id,'type',OLD.type,
      'original_title',OLD.original_title,'original_author',OLD.original_author
    )
  );
END$$

DELIMITER ;


-- =====================================
-- 5. 存储过程控制下的更新操作
-- =====================================
DELIMITER $$
CREATE PROCEDURE sp_update_rating(
  IN p_anime_id INT,
  IN p_new_rating DECIMAL(3,1)
)
BEGIN
  -- 验证评分范围
  IF p_new_rating < 0 OR p_new_rating > 10 THEN
    SIGNAL SQLSTATE '45000'
      SET MESSAGE_TEXT='评分必须在 0 到 10 之间';
  END IF;
  -- 执行更新
  UPDATE anime
    SET rating = p_new_rating
    WHERE anime_id = p_anime_id;
  -- 写审计日志
  INSERT INTO operation_audit(table_name, operation, record_id, new_data)
    VALUES(
      'anime','UPDATE',CAST(p_anime_id AS CHAR),
      JSON_OBJECT('rating', p_new_rating)
    );
END$$
DELIMITER ;

-- =====================================
-- 6. 初始数据（可选）
-- =====================================

INSERT IGNORE INTO tag (name) VALUES
  ('原创'),
  ('改编'),
  ('特摄'),
  ('布袋戏'),
  ('热血'),
  ('穿越'),
  ('奇幻'),
  ('战斗'),
  ('搞笑'),
  ('日常'),
  ('科幻'),
  ('萌系'),
  ('治愈'),
  ('校园'),
  ('少儿'),
  ('泡面'),
  ('恋爱'),
  ('少女'),
  ('魔法'),
  ('冒险'),
  ('历史'),
  ('架空'),
  ('机战'),
  ('神魔'),
  ('声控'),
  ('运动'),
  ('励志'),
  ('音乐'),
  ('推理'),
  ('社团'),
  ('智斗'),
  ('催泪'),
  ('美食'),
  ('偶像'),
  ('乙女'),
  ('职场');


