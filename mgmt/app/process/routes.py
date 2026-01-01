from flask import Blueprint, render_template, jsonify, request
from pmc import PmcComm


process_bp = Blueprint('process', __name__, template_folder='templates/features/process.html')

pmc = PmcComm('http://localhost:8012')
pmc.Set_RSA_Pri_Key("private_key.pem")
pmc.Try_Get_AES_Key()


@process_bp.after_request
def add_cors_headers(response):
    response.headers['Access-Control-Allow-Origin'] = '*'  # 允许所有来源跨域访问，如需限制可替换为具体域名
    response.headers['Access-Control-Allow-Headers'] = 'Content-Type,Authorization'
    response.headers['Access-Control-Allow-Methods'] = 'GET,POST,PUT,DELETE,OPTIONS'
    return response


@process_bp.route('/api/mgmt/process', methods=['GET', 'POST'])
def process():
    pid = []
    status = []
    order = []
    ls = pmc.get_list()
    if ls != "":
        ls_1 = ls.split('\n')
        for l in ls_1:
           i, j, k = l.split('\t')
           pid.append(i)
           status.append(j)
           order.append(k)
    data = [{"pid": p, "status": s, "order": o} for p, s, o in zip(pid, status, order)]

    return render_template('features/process.html', response=data)


@process_bp.route('/kill_process<int:pid>', methods=['POST'])
def kill_process(pid):
    pmc.kill(pid)
    return jsonify({"status": "success"})


@process_bp.route('/clear_process', methods=['POST'])
def clear_process():
    pmc.clear()
    return jsonify({"status": "success"})


@process_bp.route('/create_process<string:name>', methods=['POST'])
def create_process(name):
    pmc.start(name)
    return jsonify({"status": "success"})
