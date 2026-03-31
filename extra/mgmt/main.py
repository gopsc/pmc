from app import register_blueprints
from flask import Flask
from waitress import serve

# 初始化Flask应用
app = Flask(__name__)

# 设置一个密钥
app.secret_key = 'secret_key'

# 注册蓝图
register_blueprints(app)


if __name__ == '__main__':
    #app.run(host='0.0.0.0', port=8009, debug=True)
    serve(app, host="0.0.0.0", port=8009)
