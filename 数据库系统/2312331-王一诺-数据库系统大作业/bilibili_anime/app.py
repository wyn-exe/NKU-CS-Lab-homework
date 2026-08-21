from flask import Flask, render_template, request, redirect, url_for, flash, abort
from flask_login import LoginManager, login_user, logout_user, login_required, UserMixin
import config, models

app = Flask(__name__)
app.config.from_object(config)
models.init_app(app)
from models import (
    get_all_anime, get_all_tags,
    get_all_studios, get_anime_by_studio,
    create_anime,
    get_seiyuu_by_name, create_seiyuu,
    link_seiyuu, link_tag,
    get_director_by_name, create_director, link_director,
    get_studio_by_name, create_studio, link_studio,
    get_original_work, create_original_work, link_adaptation,
    get_all_tags, get_all_seiyuu, get_all_directors, get_all_studios ,get_studios_for_anime ,get_audit_logs,
    call_sp_update_rating
)

login_manager = LoginManager(app)
login_manager.login_view = 'login'

class User(UserMixin):
    USERS = {'admin':'123456'}
    def __init__(self, id): self.id = id

@login_manager.user_loader
def load_user(user_id):
    return User(user_id) if user_id in User.USERS else None

@app.route('/login', methods=['GET','POST'])
def login():
    if request.method=='POST':
        u = request.form['username']; p = request.form['password']
        if u in User.USERS and User.USERS[u]==p:
            login_user(User(u)); return redirect(url_for('index'))
        flash('用户名或密码错误')
    return render_template('login.html')

@app.route('/logout')
@login_required
def logout():
    logout_user(); return redirect(url_for('login'))

@app.route('/')
@login_required
def index():
    # 解析筛选和排序参数
    region = request.args.get('region') or None
    sort   = request.args.get('sort')   or None
    desc   = bool(request.args.get('desc'))
    search = request.args.get('search') or None
    tag    = request.args.get('tag')    or None

    filters = {}
    if region:
        filters['region'] = region

    anime_list = get_all_anime(
        filters=filters,
        order_by=sort,
        desc=desc,
        search=search,
        tag=tag
    )
    tags    = get_all_tags()
    return render_template(
        'index.html',
        anime_list=anime_list,
        tags=tags,
        request=request
    )

# 查看详情
@app.route('/anime/<int:anime_id>')
@login_required
def detail(anime_id):
    anime = models.get_anime_by_id(anime_id)
    tags = models.get_tags_for_anime(anime_id)
    directors = models.get_directors_for_anime(anime_id)
    seiyuus = models.get_seiyuu_for_anime(anime_id)
    adaptations = models.get_adaptations_for_anime(anime_id)
    all_directors = models.get_all_directors()
    all_seiyuu = models.get_all_seiyuu()
    all_originals = models.get_all_originals()
    return render_template(
        'detail.html', anime=anime, tags=tags,
        directors=directors, seiyuus=seiyuus,
        adaptations=adaptations,
        all_directors=all_directors,
        all_seiyuu=all_seiyuu,
        all_originals=all_originals
    )

# 新增/编辑表单复用
@app.route('/create', methods=['GET', 'POST'])
@login_required
def create():
    if request.method == 'GET':
        return render_template('form.html',
            all_tags=get_all_tags(),
            all_seiyuu=get_all_seiyuu(),
            all_directors=get_all_directors(),
            all_studios=get_all_studios(),
            anime=None
        )

    # 1) 主表插入
    data = {
        'title':   request.form['title'],
        'year':    request.form['year'],
        'region':  request.form['region'],
        'views':   request.form['views'],
        'rating':  request.form.get('rating') or None
    }
    new_id = create_anime(data)

    # 2) 三个声优：查（by name）或增， 然后关联
    for idx in (1,2,3):
        name = request.form.get(f'seiyuu_name_{idx}', '').strip()
        if name:
            seiyuu = get_seiyuu_by_name(name)
            sid = seiyuu['seiyuu_id'] if seiyuu else create_seiyuu(name)
            link_seiyuu(new_id, sid)

    # 3) 三个标签（
    for idx in (1,2,3):
        tag = request.form.get(f'tag_{idx}')
        if tag:
            link_tag(new_id, tag)

    # 4) 导演：查或增
    dname = request.form.get('director_name', '').strip()
    if dname:
        director = get_director_by_name(dname)
        did = director['director_id'] if director else create_director(dname)
        link_director(new_id, did)

    # 5) 制作单位：查或增
    sname = request.form.get('studio_name', '').strip()
    if sname:
        studio = get_studio_by_name(sname)
        staname = studio['name'] if studio else create_studio(sname)
        link_studio(new_id, staname)

    # 6) 改编
    if request.form.get('is_adapted') == '1':
        title0  = request.form.get('orig_title','').strip()
        author0 = request.form.get('orig_author','').strip()
        region0 = request.form.get('orig_region','').strip() or None
        atype   = request.form.get('adapt_type','').strip() or ''

        if title0 and author0:
            # —— 确保 original_work 一定存在 —— #
            orig = get_original_work(title0, author0)
            if not orig:
                create_original_work(title0, author0, region0)
            # 然后才关联 adapted_anime & adaptation
            link_adaptation(new_id, title0, author0, atype)

    flash('新增番剧及关联数据已保存')
    return redirect(url_for('index'))

@app.route('/anime/<int:anime_id>/edit', methods=['GET','POST'])
@login_required
def edit(anime_id):
    anime = models.get_anime_by_id(anime_id)
    if not anime:
        abort(404)

    if request.method == 'POST':
        # 1) 更新主表
        data = {
            'title':  request.form['title'],
            'year':   request.form['year'],
            'region': request.form['region'],
            'views':  request.form['views'],
            'rating': request.form.get('rating') or None
        }
        models.update_anime(anime_id, data)

        # 2) 同步声优关联（3个）
        #    先删光旧关联，再按新输入重建
        old_seiyuus = models.get_seiyuu_for_anime(anime_id)
        for s in old_seiyuus:
            models.unlink_seiyuu(anime_id, s['seiyuu_id'])
        for i in (1,2,3):
            name = request.form.get(f'seiyuu_name_{i}','').strip()
            if name:
                rec = models.get_seiyuu_by_name(name)
                sid = rec['seiyuu_id'] if rec else models.create_seiyuu(name)
                models.link_seiyuu(anime_id, sid)

        # 3) 同步标签关联（3个）
        old_tags = models.get_tags_for_anime(anime_id)
        for t in old_tags:
            models.unlink_tag(anime_id, t)
        for i in (1,2,3):
            tag = request.form.get(f'tag_{i}')
            if tag:
                models.link_tag(anime_id, tag)

        # 4) 同步导演关联
        old_dirs = models.get_directors_for_anime(anime_id)
        for d in old_dirs:
            models.unlink_director(anime_id, d['director_id'])
        dname = request.form.get('director_name','').strip()
        if dname:
            rec = models.get_director_by_name(dname)
            did = rec['director_id'] if rec else models.create_director(dname)
            models.link_director(anime_id, did)

        # 5) 同步制作单位关联
        old_studios = models.get_studios_for_anime(anime_id) 
        for studio_name in old_studios:
            models.unlink_studio(anime_id, studio_name)
        new_studio = request.form.get('studio_name','').strip()
        if new_studio:
            rec = models.get_studio_by_name(new_studio)
            name = rec['name'] if rec else models.create_studio(new_studio)
            models.link_studio(anime_id, name)

        # 6) 同步改编信息
        #    先删旧的 adaptation & 子类 adapted_anime
        old_adaps = models.get_adaptations_for_anime(anime_id)
        for a in old_adaps:
            models.unlink_adaptation(anime_id, a['original_title'], a['original_author'])
        #    如果选“是”，再按新输入关联
        if request.form.get('is_adapted') == '1':
            title0  = request.form.get('orig_title','').strip()
            author0 = request.form.get('orig_author','').strip()
            region0 = request.form.get('orig_region','').strip() or None
            atype   = request.form.get('adapt_type','').strip() or ''
            if title0 and author0:
                # 确保 original_work 存在
                orig = models.get_original_work(title0, author0)
                if not orig:
                    models.create_original_work(title0, author0, region0)
                models.link_adaptation(anime_id, title0, author0, atype)

        flash('番剧及其关联数据已更新')
        return redirect(url_for('detail', anime_id=anime_id))

    # GET：渲染表单并传入所有下拉/输入所需数据，以及现有关联
    # 取出已有关联以便在模板里填充初始值
    linked_seiyuus = [s['name'] for s in models.get_seiyuu_for_anime(anime_id)]
    linked_tags    = models.get_tags_for_anime(anime_id)
    linked_dirs    = [d['name'] for d in models.get_directors_for_anime(anime_id)]
    linked_studios = get_studios_for_anime(anime_id) 
    adaps          = models.get_adaptations_for_anime(anime_id)

    return render_template('form.html',
        action=url_for('edit', anime_id=anime_id),
        anime=anime,
        all_tags=get_all_tags(),
        # 其余 all_* 仅用于 “新增” 时下拉提示，可选传入或留空
        all_seiyuu=[], all_directors=[], all_studios=[],
        # 传现有关联以便在表单里 pre-fill
        linked_seiyuus=linked_seiyuus,
        linked_tags=linked_tags,
        linked_dirs=linked_dirs,
        linked_studios=linked_studios,
        linked_adaptations=adaps
    )

#更新评分
@app.route('/anime/<int:anime_id>/rating', methods=['POST'])
@login_required
def update_rating(anime_id):
    """
    使用存储过程更新评分
    """
    # 参数校验
    try:
        new_rating = float(request.form.get('new_rating'))
    except (TypeError, ValueError):
        flash('请输入合法的评分数值', 'danger')
        return redirect(url_for('detail', anime_id=anime_id))

    try:
        models.call_sp_update_rating(anime_id, new_rating)
        flash('评分已更新（via 存储过程）', 'success')
    except Exception as e:
        flash(f'更新失败：{e}', 'danger')

    return redirect(url_for('detail', anime_id=anime_id))

# 删除
@app.route('/anime/<int:anime_id>/delete')
@login_required
def delete(anime_id):
    models.delete_anime(anime_id)
    flash('删除成功')
    return redirect(url_for('index'))

# 标签管理
@app.route('/anime/<int:anime_id>/tag/add', methods=['POST'])
@login_required
def add_tag(anime_id):
    name = request.form['tag']
    models.link_tag(anime_id, name)
    return redirect(url_for('detail', anime_id=anime_id))

@app.route('/anime/<int:anime_id>/tag/<tag_name>/remove')
@login_required
def remove_tag(anime_id, tag_name):
    models.unlink_tag(anime_id, tag_name)
    return redirect(url_for('detail', anime_id=anime_id))

# 制作单位列表
@app.route('/studios')
@login_required
def studios():
    studios = models.get_all_studios()
    return render_template('studios.html', studios=studios)

@app.route('/studios/<studio_name>')
@login_required
def studio_detail(studio_name):
    """
    制作单位详情：展示该单位的所有番剧
    """
    studios = get_all_studios()  # 用于导航左侧或回链
    anime_list = get_anime_by_studio(studio_name)
    return render_template(
        'studio_detail.html',
        studio_name=studio_name,
        anime_list=anime_list,
        studios=studios
    )

# 搜索
@app.route('/search')
@login_required
def search():
    q = request.args.get('q')
    anime_list = []
    if q:
        anime_list = models.get_all_anime({'title':q})
    return render_template('index.html', anime_list=anime_list)

# ---------- 导演关联 ----------
@app.route('/anime/<int:anime_id>/director/add', methods=['POST'])
@login_required
def add_director(anime_id):
    director_id = request.form['director_id']
    models.link_director(anime_id, director_id)
    return redirect(url_for('detail', anime_id=anime_id))

@app.route('/anime/<int:anime_id>/director/<int:director_id>/remove')
@login_required
def remove_director(anime_id, director_id):
    models.unlink_director(anime_id, director_id)
    return redirect(url_for('detail', anime_id=anime_id))

# ---------- 声优关联 ----------
@app.route('/anime/<int:anime_id>/seiyuu/add', methods=['POST'])
@login_required
def add_seiyuu(anime_id):
    seiyuu_id = request.form['seiyuu_id']
    models.link_seiyuu(anime_id, seiyuu_id)
    return redirect(url_for('detail', anime_id=anime_id))

@app.route('/anime/<int:anime_id>/seiyuu/<int:seiyuu_id>/remove')
@login_required
def remove_seiyuu(anime_id, seiyuu_id):
    models.unlink_seiyuu(anime_id, seiyuu_id)
    return redirect(url_for('detail', anime_id=anime_id))

# ---------- 改编关联 ----------
@app.route('/anime/<int:anime_id>/adaptation/add', methods=['POST'])
@login_required
def add_adaptation(anime_id):
    title = request.form['original_title']
    author = request.form['original_author']
    atype = request.form['adaptation_type']
    models.link_adaptation(anime_id, title, author, atype)
    return redirect(url_for('detail', anime_id=anime_id))

@app.route('/anime/<int:anime_id>/adaptation/remove')
@login_required
def remove_adaptation(anime_id):
    title = request.args['original_title']
    author = request.args['original_author']
    models.unlink_adaptation(anime_id, title, author)
    return redirect(url_for('detail', anime_id=anime_id))


# 排行榜：播放量 Top10
@app.route('/rank/views')
@login_required
def rank_views():
    top = models.get_top10_by_views()
    return render_template('rank_views.html', top=top)

# 排行榜：评分 Top10
@app.route('/rank/rating')
@login_required
def rank_rating():
    top = models.get_top10_by_rating()
    return render_template('rank_rating.html', top=top)

# 制作单位统计
@app.route('/stats/studios')
@login_required
def stats_studios():
    counts = models.get_studio_counts()
    return render_template('stats_studios.html', counts=counts)

# 播放量历史
@app.route('/anime/<int:anime_id>/views_log')
@login_required
def views_log(anime_id):
    logs = models.get_views_log(anime_id)
    return render_template('views_log.html', logs=logs, anime_id=anime_id)

@app.route('/audit')
@login_required
def audit():
    """
    审计日志展示页面
    """
    logs = get_audit_logs()
    return render_template('audit.html', logs=logs)

if __name__=='__main__':
    app.run(debug=True)

