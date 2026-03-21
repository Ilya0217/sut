"""
Модуль: users_api.py
Назначение: REST API управления пользователями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.Security.UserManagement
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required

from services.user_service import UserError, change_user_role, get_all_users

users_api = Blueprint("users_api", __name__, url_prefix="/api/users")


@users_api.route("", methods=["GET"])
@login_required
def list_users():
    """Назначение: Список пользователей."""
    users = get_all_users()
    return jsonify([
        {
            "id": u.id,
            "username": u.username,
            "email": u.email,
            "role": u.role,
            "created_at": u.created_at.isoformat() if u.created_at else None,
        }
        for u in users
    ])


@users_api.route("/<int:uid>/role", methods=["PUT"])
@login_required
def update_role(uid):
    """Назначение: Изменение роли пользователя."""
    if not current_user.can_manage_users():
        return jsonify({"error": "Доступ запрещён"}), 403

    data = request.get_json(silent=True) or {}
    try:
        user = change_user_role(uid, data.get("role", ""), current_user.id)
        return jsonify({
            "id": user.id,
            "username": user.username,
            "role": user.role,
        })
    except UserError as exc:
        return jsonify({"error": exc.message}), 400
