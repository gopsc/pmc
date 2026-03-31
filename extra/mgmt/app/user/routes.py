import jwt
import hashlib
from flask import Blueprint, render_template, request, current_app, jsonify
from pmc import DnfsComm
from dnfs import UsersTable, DnfsTable

user_bp = Blueprint('user', __name__, template_folder='templates/features/user.html')

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


def get_node():
    try:
        data = comm.get_and_trans(0)
    except DnfsComm.GetFailed:
        return "获取数据失败", 500
    for item in data:
        if item['title'] == 'Users':
            node_id = item['id']
            node_content = item['content']
            return node_id, node_content
    return None


@user_bp.after_request
def add_cors_headers(response):
    response.headers['Access-Control-Allow-Origin'] = '*'  # 允许所有来源跨域访问，如需限制可替换为具体域名
    response.headers['Access-Control-Allow-Headers'] = 'Content-Type,Authorization'
    response.headers['Access-Control-Allow-Methods'] = 'GET,POST,PUT,DELETE,OPTIONS'
    return response


@user_bp.route('/api/mgmt/user', methods=['GET', 'POST'])
def user():
    try:
        data = comm.get_and_trans(0)
        dnfs_tab = DnfsTable(data)
        users = dnfs_tab.query_by_title('Users')
        table = UsersTable(users.content)
        crt_dict = lambda user: {'user_id': user.id, 'username': user.name, 'role': user.privileges}
        user_list = [crt_dict(user) for user in table.arr]
    except DnfsComm.GetFailed:
        return "获取数据失败", 500
    except DnfsTable.NotFound:
        return "未找到用户表", 404
    except UsersTable.NotFound:
        return "用户不存在", 401
    except Exception as e:
        return str(e), 404
    payload = get_user_from_token()
    if not payload:
        return jsonify({"error": "未授权访问"}), 401

    user_id = payload.get('user_id')
    role = payload.get('role')

    return render_template('features/user.html',
                           response=user_list,
                           current_user_id=user_id,
                           current_role=role)


@user_bp.route('/add_user', methods=['POST'])
def add_user():
    node_id, msg = get_node()
    username = request.form.get('username')
    password = request.form.get('password')
    hashed_password = hashlib.sha256(password.encode("utf-8")).hexdigest()
    role = "normal"
    table = UsersTable(msg)
    table.add_user(username, hashed_password, role)
    table = str(table)
    comm.upd(node_id, 0, 'Users', table)
    return jsonify({"message": "用户添加成功"})


@user_bp.route('/change_password', methods=['POST'])
def change_password():
    node_id, msg = get_node()
    user_id = request.form.get('user_id')
    new_password = request.form.get('new_password')
    hashed_password = hashlib.sha256(new_password.encode("utf-8")).hexdigest()
    table = UsersTable(msg)
    table.change_password(int(user_id), hashed_password)
    table = str(table)
    comm.upd(node_id, 0, 'Users', table)
    return jsonify({"message": "密码修改成功"})


@user_bp.route('/delete_user<int:user_id>', methods=['POST'])
def delete_user(user_id):
    node_id, msg = get_node()
    table = UsersTable(msg)
    table.del_user(user_id)
    table = str(table)
    comm.upd(node_id, 0, 'Users', table)
    return jsonify({"message": "用户删除成功"})
