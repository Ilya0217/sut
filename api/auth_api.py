"""
Модуль: auth_api.py
Назначение: REST API аутентификации и авторизации
Автор: Разработчик
Дата создания: 21.03.2026
Требования: LLR_SecurityManager_Authenticate_01, Functional.Security.UserManagement
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required, login_user, logout_user

from services.audit_service import log_action
from services.user_service import UserError, authenticate_user, register_user

auth_api = Blueprint("auth_api", __name__, url_prefix="/api/auth")


@auth_api.route("/login", methods=["POST"])
def login():
    """
    Назначение: Аутентификация пользователя
    Id требования: LLR_SecurityManager_Authenticate_01
    Входные данные: JSON {username, password}
    Выходные данные: JSON с данными пользователя
    """
    data = request.get_json(silent=True) or {}
    username = data.get("username", "").strip()
    password = data.get("password", "")

    if not username or not password:
        return jsonify({"error": "Имя пользователя и пароль обязательны"}), 400

    user = authenticate_user(username, password)
    if user is None:
        return jsonify({"error": "Неверное имя пользователя или пароль"}), 401

    login_user(user)
    return jsonify({
        "id": user.id,
        "username": user.username,
        "email": user.email,
        "role": user.role,
    })


@auth_api.route("/register", methods=["POST"])
def register():
    """
    Назначение: Регистрация нового пользователя
    Id требования: Functional.Security.UserManagement
    Входные данные: JSON {username, email, password, role}
    Выходные данные: JSON с данными пользователя
    """
    data = request.get_json(silent=True) or {}
    try:
        user = register_user(
            username=data.get("username", "").strip(),
            email=data.get("email", "").strip(),
            password=data.get("password", ""),
            role=data.get("role", "analyst"),
        )
        login_user(user)
        return jsonify({
            "id": user.id,
            "username": user.username,
            "email": user.email,
            "role": user.role,
        }), 201
    except UserError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@auth_api.route("/logout", methods=["POST"])
@login_required
def logout():
    """Назначение: Выход из системы."""
    logout_user()
    return jsonify({"message": "OK"})


@auth_api.route("/me", methods=["GET"])
@login_required
def me():
    """Назначение: Получение данных текущего пользователя."""
    return jsonify({
        "id": current_user.id,
        "username": current_user.username,
        "email": current_user.email,
        "role": current_user.role,
    })
