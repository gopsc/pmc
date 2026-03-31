from pmc import PmcComm
from flask import Blueprint, render_template

module_bp = Blueprint('module', __name__, template_folder='templates/features/module.html')

pmc = PmcComm('http://localhost:8012')
pmc.Set_RSA_Pri_Key("private_key.pem")
pmc.Try_Get_AES_Key()


@module_bp.after_request
def add_cors_headers(response):
    response.headers['Access-Control-Allow-Origin'] = '*'  # 允许所有来源跨域访问，如需限制可替换为具体域名
    response.headers['Access-Control-Allow-Headers'] = 'Content-Type,Authorization'
    response.headers['Access-Control-Allow-Methods'] = 'GET,POST,PUT,DELETE,OPTIONS'
    return response


@module_bp.route('/api/mgmt/module', methods=['GET', 'POST'])
def module():
    data = pmc.list_mods()
    modules = data.split('\n')
    return render_template('features/module.html', modules=modules)


@module_bp.route('/create_module<string:mod_name>', methods=['GET', 'POST'])
def create_module(mod_name):
    pass


@module_bp.route('/delete_module<string:mod_name>', methods=['GET', 'POST'])
def delete_module(mod_name):
    pass

