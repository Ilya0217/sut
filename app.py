"""
Модуль: app.py
Назначение: Точка входа веб-приложения СУТ (Система управления требованиями)
Автор: Разработчик
Дата создания: 20.03.2026
Изменения: 21.03.2026 — переход на REST API + Vue.js фронтенд
Требования: Архитектура ПО, Interface.Software.APIGetProjects
"""

import sys
import os

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from flask import Flask, jsonify
from flask_login import LoginManager

from config import Config
from models import User, db


def create_app():
    """
    Назначение: Создание и настройка экземпляра приложения Flask
    Id требования: Архитектура ПО (ApiGateway)
    Входные данные: нет
    Выходные данные: экземпляр Flask приложения
    """
    app = Flask(__name__)
    app.config.from_object(Config)

    db.init_app(app)

    login_manager = LoginManager()
    login_manager.init_app(app)

    @login_manager.user_loader
    def load_user(user_id):
        return db.session.get(User, int(user_id))

    @login_manager.unauthorized_handler
    def unauthorized():
        return jsonify({"error": "Требуется аутентификация"}), 401

    from api.auth_api import auth_api
    from api.projects_api import projects_api
    from api.requirements_api import req_api
    from api.tracelinks_api import trace_api
    from api.import_export_api import ie_api
    from api.changes_api import changes_api
    from api.notifications_api import notif_api
    from api.audit_api import audit_api
    from api.users_api import users_api

    app.register_blueprint(auth_api)
    app.register_blueprint(projects_api)
    app.register_blueprint(req_api)
    app.register_blueprint(trace_api)
    app.register_blueprint(ie_api)
    app.register_blueprint(changes_api)
    app.register_blueprint(notif_api)
    app.register_blueprint(audit_api)
    app.register_blueprint(users_api)

    @app.after_request
    def add_cors_headers(response):
        origin = os.environ.get("CORS_ORIGIN", "http://localhost:5173")
        response.headers["Access-Control-Allow-Origin"] = origin
        response.headers["Access-Control-Allow-Credentials"] = "true"
        response.headers["Access-Control-Allow-Headers"] = "Content-Type"
        response.headers["Access-Control-Allow-Methods"] = "GET,POST,PUT,DELETE,OPTIONS"
        return response

    @app.route("/api/health")
    def health():
        return jsonify({"status": "ok"})

    with app.app_context():
        db.create_all()
        create_default_admin()

    return app


def create_default_admin():
    """
    Назначение: Создание администратора по умолчанию при первом запуске
    Входные данные: нет
    Выходные данные: нет
    """
    if User.query.filter_by(username="admin").first() is None:
        admin = User(
            username="admin",
            email="admin@sut.local",
            role="admin",
        )
        admin.set_password("admin123")
        db.session.add(admin)
        db.session.commit()
        print("Создан администратор: admin / admin123")


app = create_app()

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=5123)
