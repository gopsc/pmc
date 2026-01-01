import datetime
import hashlib
from functools import wraps

from pmc import DnfsComm
from dnfs import UsersTable, DnfsTable

import jwt
from flask import Blueprint, render_template, request, redirect, url_for, current_app, make_response, jsonify

login_bp = Blueprint('login', __name__, template_folder='templates/login')


comm = DnfsComm("http://localhost:8006")


def get_user_from_token():
    token = request.cookies.get('token')
    if not token:
        return None
    try:
        payload = jwt.decode(token, current_app.config['SECRET_KEY'], algorithms=['HS256'])
        return payload  # 包含 user、user_id、exp 等字段
    except jwt.ExpiredSignatureError:
        return None
    except jwt.InvalidTokenError:
        return None


@login_bp.after_request
def add_cors_headers(response):
    response.headers['Access-Control-Allow-Origin'] = '*'  # 允许所有来源跨域访问，如需限制可替换为具体域名
    response.headers['Access-Control-Allow-Headers'] = 'Content-Type,Authorization'
    response.headers['Access-Control-Allow-Methods'] = 'GET,POST,PUT,DELETE,OPTIONS'
    return response


def token_required(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        # 从cookie中获取token
        token = request.cookies.get('token')
        if not token:
            return redirect(url_for('login.index'))
        try:
            jwt.decode(token, current_app.config['SECRET_KEY'], algorithms=['HS256'])
        except jwt.ExpiredSignatureError:
            return redirect(url_for('login.index'))
        except jwt.InvalidTokenError:
            return redirect(url_for('login.index'))
        return f(*args, **kwargs)
    return decorated


@login_bp.route('/')
def index():
    return render_template('index.html')


@login_bp.route('/api/login', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form.get('user-name')
        password = request.form.get('user-passwd')
        password = password.encode("utf-8")
        hashed_password = hashlib.sha256(password).hexdigest()
        try:
            data = comm.get_and_trans(0)
            dnfs_tab = DnfsTable(data)
            users = dnfs_tab.query_by_title('Users')
            table = UsersTable(users.content)
            data = table.user_exists(username, hashed_password)
            if not data:
                return "用户登录失败", 401
            _user_id, _username, _password, _role = data
        except DnfsComm.GetFailed:
            return "获取数据失败", 500
        except DnfsTable.NotFound:
            return "未找到用户表", 404
        except UsersTable.NotFound:
            return "用户不存在", 401
        except Exception as e:
            return str(e), 404
        # 生成 Token
        token = jwt.encode({
            'user_id': _user_id,
            'username': _username,
            'role': _role,
            'exp': datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=3)
        }, current_app.config['SECRET_KEY'], algorithm='HS256')

        # 构造 JSON 响应体
        response = make_response(jsonify({
            "redirect": url_for('login.mgmt'),  # 跳转路径
            "token": token
        }))

        # 设置 Cookie
        response.set_cookie('token', token)
        return response
    else:
        return "登录失败"


@login_bp.route('/api/mgmt')
@token_required
def mgmt():
    payload = get_user_from_token()
    if not payload:
        return redirect(url_for('login.index'))
    username = payload.get('username')
    role = payload.get('role')
    return render_template('mgmt.html', username=username, role=role)
