-- 实体表
CREATE TABLE 番剧 (
  ID INT PRIMARY KEY,
  上映年份 YEAR,
  名称 VARCHAR(100) NOT NULL,
  播放量 BIGINT,
  评分 DECIMAL(3,1),
  地区 VARCHAR(50)
);

CREATE TABLE 制作单位 (
  名称 VARCHAR(100) PRIMARY KEY,
  代表作 VARCHAR(255)
);

CREATE TABLE 标签 (
  标签名 VARCHAR(50) PRIMARY KEY
);

CREATE TABLE 导演 (
  ID INT PRIMARY KEY,
  人名 VARCHAR(50) NOT NULL,
  国籍 VARCHAR(50),
  代表作 VARCHAR(255)
);

CREATE TABLE 声优 (
  ID INT PRIMARY KEY,
  姓名 VARCHAR(50) NOT NULL,
  代表作 VARCHAR(255),
  国籍 VARCHAR(50)
);

CREATE TABLE 原作 (
  名称 VARCHAR(100),
  作者 VARCHAR(100),
  地区 VARCHAR(50),
  PRIMARY KEY (名称, 作者)
);

-- 子类表（继承自番剧）
CREATE TABLE 改编动画 (
  ID INT PRIMARY KEY,
  原作名称 VARCHAR(100),
  FOREIGN KEY (ID) REFERENCES 番剧(ID)
);

-- 关系表
CREATE TABLE 制作 (
  番剧ID INT,
  制作单位名称 VARCHAR(100),
  PRIMARY KEY (番剧ID, 制作单位名称),
  FOREIGN KEY (番剧ID) REFERENCES 番剧(ID),
  FOREIGN KEY (制作单位名称) REFERENCES 制作单位(名称)
);

CREATE TABLE 改编 (
  番剧ID INT,
  改编类型 VARCHAR(50),
  原作名称 VARCHAR(100),
  原作作者 VARCHAR(100),
  PRIMARY KEY (番剧ID, 原作名称, 原作作者),
  FOREIGN KEY (番剧ID) REFERENCES 改编动画(ID),
  FOREIGN KEY (原作名称, 原作作者) REFERENCES 原作(名称, 作者)
);

CREATE TABLE 导片 (
  导演ID INT,
  番剧ID INT,
  PRIMARY KEY (导演ID, 番剧ID),
  FOREIGN KEY (导演ID) REFERENCES 导演(ID),
  FOREIGN KEY (番剧ID) REFERENCES 番剧(ID)
);

CREATE TABLE 标签关联 (
  标签名 VARCHAR(50),
  番剧ID INT,
  PRIMARY KEY (标签名, 番剧ID),
  FOREIGN KEY (标签名) REFERENCES 标签(标签名),
  FOREIGN KEY (番剧ID) REFERENCES 番剧(ID)
);

CREATE TABLE 参演 (
  声优ID INT,
  番剧ID INT,
  PRIMARY KEY (声优ID, 番剧ID),
  FOREIGN KEY (声优ID) REFERENCES 声优(ID),
  FOREIGN KEY (番剧ID) REFERENCES 番剧(ID)
);


-- 单表查询（查询评分最高的5部番剧）
SELECT 名称, 评分
FROM 番剧
ORDER BY 评分 DESC
LIMIT 5;

-- 多表连接查询（查询每部番剧的导演和制作单位）
SELECT f.名称 AS 番剧名称, d.人名 AS 导演, p.名称 AS 制作单位
FROM 番剧 f
JOIN 导片 df ON f.ID = df.番剧ID
JOIN 导演 d ON df.导演ID = d.ID
JOIN 制作 m ON f.ID = m.番剧ID
JOIN 制作单位 p ON m.制作单位名称 = p.名称;

-- 多表嵌套查询（查询是否存在没有声优参演的番剧）
SELECT 名称
FROM 番剧
WHERE ID NOT IN (
    SELECT DISTINCT 番剧ID
    FROM 参演
);

-- EXISTS查询（查询参演过评分9分以上番剧的声优）
SELECT v.姓名, v.代表作
FROM 声优 v
WHERE EXISTS (
    SELECT 1
    FROM 参演 c
    JOIN 番剧 f ON c.番剧ID = f.ID
    WHERE c.声优ID = v.ID AND f.评分 >= 9
);

-- 聚合操作查询（统计各地区番剧的平均评分和数量）
SELECT 地区, 
       AVG(评分) AS 平均评分,
       COUNT(*) AS 番剧数量
FROM 番剧
GROUP BY 地区
HAVING AVG(评分) > 7
ORDER BY 平均评分 DESC;