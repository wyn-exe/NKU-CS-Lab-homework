from flask_mysqldb import MySQL

# 初始化 MySQL 连接
db = MySQL()

def init_app(app):
    """
    将 MySQL 实例绑定到 Flask 应用
    """
    db.init_app(app)

# ---------- 番剧（anime）操作 ----------

def get_all_anime(filters=None, order_by=None, desc=False, search=None, tag=None):
    """
    获取所有番剧，支持：
      - filters: 字典过滤，如 {'region': '日本'}
      - search: 按名称模糊搜索
      - tag: 按标签过滤
      - order_by + desc: 排序
    """
    sql = 'SELECT DISTINCT a.* FROM anime a'
    params = []

    # 如果按标签过滤，则 JOIN 标签关系表
    if tag:
        sql += ' JOIN anime_tag at ON a.anime_id = at.anime_id'

    sql += ' WHERE 1=1'

    # 地区或其他精确过滤
    if filters:
        for k, v in filters.items():
            sql += f' AND a.{k} = %s'
            params.append(v)

    # 名称模糊搜索
    if search:
        sql += ' AND a.title LIKE %s'
        params.append(f'%{search}%')

    # 标签过滤
    if tag:
        sql += ' AND at.tag_name = %s'
        params.append(tag)

    # 排序
    if order_by:
        sql += f' ORDER BY a.{order_by} {"DESC" if desc else "ASC"}'

    cur = db.connection.cursor()
    cur.execute(sql, params)
    return cur.fetchall()

def get_anime_by_id(anime_id):
    """
    根据 ID 获取单个番剧
    """
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM anime WHERE anime_id=%s', (anime_id,))
    return cur.fetchone()

def create_anime(data):
    """
    插入新番剧
    :param data: dict, 包含 title, year, region, views, rating
    :return: 插入记录的 ID
    """
    keys = ','.join(data.keys())
    placeholders = ','.join(['%s'] * len(data))
    sql = f"INSERT INTO anime ({keys}) VALUES ({placeholders})"
    cur = db.connection.cursor()
    cur.execute(sql, tuple(data.values()))
    db.connection.commit()
    return cur.lastrowid

def update_anime(anime_id, data):
    """
    更新番剧信息
    """
    assignments = ','.join(f"{k}=%s" for k in data)
    sql = f"UPDATE anime SET {assignments} WHERE anime_id=%s"
    params = list(data.values()) + [anime_id]
    cur = db.connection.cursor()
    cur.execute(sql, params)
    db.connection.commit()

def delete_anime(anime_id):
    """
    删除番剧
    """
    cur = db.connection.cursor()
    cur.execute('DELETE FROM anime WHERE anime_id=%s', (anime_id,))
    db.connection.commit() # 隐式事务提交

def call_sp_update_rating(anime_id, new_rating):
    """
    调用存储过程更新番剧评分
    """
    cur = db.connection.cursor()
    # 调用存储过程
    cur.callproc('sp_update_rating', (anime_id, new_rating))
    db.connection.commit()


# ---------- 标签（tag）操作 ----------

def get_all_tags():
    """
    获取所有标签
    """
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM tag')
    return cur.fetchall()

def add_tag(name):
    """
    新增标签（若存在则忽略）
    """
    cur = db.connection.cursor()
    cur.execute('INSERT IGNORE INTO tag(name) VALUES(%s)', (name,))
    db.connection.commit()

def get_tags_for_anime(anime_id):
    """
    获取指定番剧的所有标签
    """
    cur = db.connection.cursor()
    cur.execute(
        'SELECT t.name FROM tag t JOIN anime_tag at ON t.name=at.tag_name WHERE at.anime_id=%s',
        (anime_id,)
    )
    return [row['name'] for row in cur.fetchall()]

def link_tag(anime_id, tag_name):
    """
    为番剧关联标签
    """
    try:
        cur = db.connection.cursor()
        # 1. 确保标签存在（若不存在则插入）
        cur.execute("INSERT IGNORE INTO tag (name) VALUES (%s)", (tag_name,))
        # 2. 插入关联记录，触发 trg_tag_increment
        cur.execute("INSERT INTO anime_tag (tag_name, anime_id) VALUES (%s, %s)", (tag_name, anime_id))
        db.connection.commit()
    except Exception as e:
        db.connection.rollback()
        raise e

def unlink_tag(anime_id, tag_name):
    """
    删除番剧标签关联
    """
    cur = db.connection.cursor()
    cur.execute('DELETE FROM anime_tag WHERE anime_id=%s AND tag_name=%s', (anime_id, tag_name))
    db.connection.commit()

# ---------- 制作单位（studio）操作 ----------

def get_all_studios():
    """
    获取所有制作单位
    """
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM studio')
    return cur.fetchall()
# 制作单位下番剧列表 
def get_anime_by_studio(studio_name):
    """
    获取指定制作单位下的所有番剧
    """
    sql = '''
      SELECT a.*
      FROM anime a
      JOIN production p ON a.anime_id = p.anime_id
      WHERE p.studio_name = %s
      ORDER BY a.views DESC
    '''
    cur = db.connection.cursor()
    cur.execute(sql, (studio_name,))
    return cur.fetchall()

def get_studios_for_anime(anime_id):
    """
    获取指定番剧关联的所有制作单位名称列表
    """
    cur = db.connection.cursor()
    cur.execute(
        'SELECT studio_name FROM production WHERE anime_id=%s',
        (anime_id,)
    )
    # 假设 cursor 设置为字典模式，则 row['studio_name']
    return [row['studio_name'] for row in cur.fetchall()]
def get_studio_by_name(name):
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM studio WHERE name=%s', (name,))
    return cur.fetchone()

def create_studio(name, flagship_work=None):
    cur = db.connection.cursor()
    cur.execute(
        'INSERT INTO studio(name, flagship_work) VALUES(%s, %s)',
        (name, flagship_work or '')
    )
    db.connection.commit()
    return name  # studio.name is primary key (string)

# ---------- 导演（director）操作 ----------

def get_all_directors():
    """
    获取所有导演
    """
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM director')
    return cur.fetchall()

def get_directors_for_anime(anime_id):
    """
    获取指定番剧的导演列表
    """
    cur = db.connection.cursor()
    cur.execute(
        'SELECT d.* FROM director d JOIN directorship ds ON d.director_id=ds.director_id WHERE ds.anime_id=%s',
        (anime_id,)
    )
    return cur.fetchall()

def get_director_by_name(name):
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM director WHERE name=%s', (name,))
    return cur.fetchone()

def create_director(name, nationality=None, flagship_work=None):
    cur = db.connection.cursor()
    cur.execute(
        'INSERT INTO director(name, nationality, flagship_work) VALUES(%s, %s, %s)',
        (name, nationality or '', flagship_work or '')
    )
    db.connection.commit()
    return cur.lastrowid

# ---------- 声优（seiyuu）操作 ----------

def get_all_seiyuu():
    """
    获取所有声优
    """
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM seiyuu')
    return cur.fetchall()

def get_seiyuu_for_anime(anime_id):
    """
    获取指定番剧的声优列表
    """
    cur = db.connection.cursor()
    cur.execute(
        'SELECT s.* FROM seiyuu s JOIN casting c ON s.seiyuu_id=c.seiyuu_id WHERE c.anime_id=%s',
        (anime_id,)
    )
    return cur.fetchall()

def get_seiyuu_by_name(name):
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM seiyuu WHERE name=%s', (name,))
    return cur.fetchone()

def create_seiyuu(name, nationality=None, flagship_work=None):
    cur = db.connection.cursor()
    cur.execute(
        'INSERT INTO seiyuu(name, nationality, flagship_work) VALUES(%s, %s, %s)',
        (name, nationality or '', flagship_work or '')
    )
    db.connection.commit()
    return cur.lastrowid

# ---------- 原作（original_work）与改编 ----------

def get_all_originals():
    """
    获取所有原作
    """
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM original_work')
    return cur.fetchall()

def get_adaptations_for_anime(anime_id):
    """
    获取指定番剧的所有改编信息
    """
    cur = db.connection.cursor()
    cur.execute(
        'SELECT * FROM adaptation WHERE anime_id=%s',
        (anime_id,)
    )
    return cur.fetchall()

def get_original_work(title, author):
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM original_work WHERE title=%s AND author=%s', (title, author))
    return cur.fetchone()

def create_original_work(title, author, region=None):
    cur = db.connection.cursor()
    cur.execute(
        'INSERT INTO original_work(title, author, region) VALUES(%s, %s, %s)',
        (title, author, region or '')
    )
    db.connection.commit()
    return {'title': title, 'author': author}

# ---------- 关系操作（production, adaptation, directorship, casting） ----------

def link_studio(anime_id, studio_name):
    """
    为番剧关联制作单位
    """
    cur = db.connection.cursor()
    cur.execute('INSERT IGNORE INTO production(anime_id,studio_name) VALUES(%s,%s)', (anime_id, studio_name))
    db.connection.commit()

def unlink_studio(anime_id, studio_name):
    """
    删除番剧与制作单位的关联
    """
    cur = db.connection.cursor()
    cur.execute('DELETE FROM production WHERE anime_id=%s AND studio_name=%s', (anime_id, studio_name))
    db.connection.commit()

# ---------- 导演-番剧 关系操作 ----------

def link_director(anime_id, director_id):
    """
    为番剧关联导演
    """
    cur = db.connection.cursor()
    cur.execute(
        'INSERT IGNORE INTO directorship(director_id, anime_id) VALUES(%s, %s)',
        (director_id, anime_id)
    )
    db.connection.commit()

def unlink_director(anime_id, director_id):
    """
    删除番剧与导演的关联
    """
    cur = db.connection.cursor()
    cur.execute(
        'DELETE FROM directorship WHERE anime_id=%s AND director_id=%s',
        (anime_id, director_id)
    )
    db.connection.commit()

# ---------- 声优-番剧 关系操作 ----------

def link_seiyuu(anime_id, seiyuu_id):
    """
    为番剧关联声优
    """
    cur = db.connection.cursor()
    cur.execute(
        'INSERT IGNORE INTO casting(seiyuu_id, anime_id) VALUES(%s, %s)',
        (seiyuu_id, anime_id)
    )
    db.connection.commit()

def unlink_seiyuu(anime_id, seiyuu_id):
    """
    删除番剧与声优的关联
    """
    cur = db.connection.cursor()
    cur.execute(
        'DELETE FROM casting WHERE anime_id=%s AND seiyuu_id=%s',
        (anime_id, seiyuu_id)
    )
    db.connection.commit()

# ---------- 改编（原作） 关系操作 ----------

def link_adaptation(anime_id, original_title, original_author, adaptation_type):
    """
    1) 确保 original_work 表中存在条目
    2) 再插入 adapted_anime、adaptation
    """
    cur = db.connection.cursor()

    # 1) 原作查或增
    cur.execute(
      'SELECT 1 FROM original_work WHERE title=%s AND author=%s',
      (original_title, original_author)
    )
    if not cur.fetchone():
        cur.execute(
          'INSERT INTO original_work(title, author, region) VALUES(%s, %s, %s)',
          (original_title, original_author, '')
        )

    # 2) 插入 adapted_anime（子类）  
    cur.execute(
        'INSERT IGNORE INTO adapted_anime(anime_id, original_title, original_author) '
        'VALUES(%s, %s, %s)',
        (anime_id, original_title, original_author)
    )
    # 3) 插入具体改编关系
    cur.execute(
        'INSERT IGNORE INTO adaptation(anime_id, type, original_title, original_author) '
        'VALUES(%s, %s, %s, %s)',
        (anime_id, adaptation_type, original_title, original_author)
    )
    db.connection.commit()

def unlink_adaptation(anime_id, original_title, original_author):
    """
    删除番剧与原作改编的关联
    """
    cur = db.connection.cursor()
    # 删除 adaptation 表中的具体记录
    cur.execute(
        'DELETE FROM adaptation WHERE anime_id=%s AND original_title=%s AND original_author=%s',
        (anime_id, original_title, original_author)
    )
    # 如果 adapted_anime 表中不再有任何 adaptation 记录，可选地同时删除子类记录
    cur.execute(
        'SELECT COUNT(*) AS cnt FROM adaptation WHERE anime_id=%s',
        (anime_id,)
    )
    cnt = cur.fetchone()['cnt']
    if cnt == 0:
        cur.execute('DELETE FROM adapted_anime WHERE anime_id=%s', (anime_id,))
    db.connection.commit()


# 获取播放量 Top10
def get_top10_by_views():
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM view_top10_by_views')
    return cur.fetchall()

# 获取评分 Top10
def get_top10_by_rating():
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM view_top10_by_rating')
    return cur.fetchall()

# 获取各制作单位番剧数量
def get_studio_counts():
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM view_studio_counts')
    return cur.fetchall()

# 获取某番剧播放量变动历史
def get_views_log(anime_id):
    cur = db.connection.cursor()
    cur.execute('SELECT * FROM anime_views_log WHERE anime_id=%s ORDER BY changed_at DESC', (anime_id,))
    return cur.fetchall()

def get_audit_logs(limit=200):
    """
    获取最新的审计日志记录
    """
    cur = db.connection.cursor()
    cur.execute(
        '''
        SELECT audit_id, table_name, operation, record_id,
               old_data, new_data, changed_at
        FROM operation_audit
        ORDER BY changed_at DESC
        LIMIT %s
        ''',
        (limit,)
    )
    return cur.fetchall()
