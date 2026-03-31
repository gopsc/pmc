from app.login.routes import login_bp
from app.log.routes import log_bp
from app.module.routes import module_bp
from app.process.routes import process_bp
from app.user.routes import user_bp


def register_blueprints(app):
    # 默认访问家目录
    app.register_blueprint(login_bp, url_prefix='/')
    app.register_blueprint(log_bp, url_prefix='/log')
    app.register_blueprint(module_bp, url_prefix='/module')
    app.register_blueprint(process_bp, url_prefix='/process')
    app.register_blueprint(user_bp, url_prefix='/user')
