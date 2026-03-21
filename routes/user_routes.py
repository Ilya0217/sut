"""
Модуль: user_routes.py
Назначение: Маршруты управления пользователями
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Functional.Security.UserManagement, Functional.Security.AccessControl
"""

from flask import Blueprint, flash, redirect, render_template, request, url_for
from flask_login import current_user, login_required

from models import User
from services.notification_service import get_unread_count
from services.user_service import UserError, change_user_role, get_all_users

user_bp = Blueprint("users", __name__)


@user_bp.route("/users")
@login_required
def list_users():
    """Назначение: Список пользователей системы."""
    if not current_user.can_manage_users():
        flash("Доступ запрещён", "error")
        return redirect(url_for("main.index"))

    users = get_all_users()
    return render_template(
        "users.html",
        users=users,
        unread_count=get_unread_count(current_user.id),
    )


@user_bp.route("/users/<int:uid>/role", methods=["POST"])
@login_required
def update_role(uid):
    """Назначение: Изменение роли пользователя."""
    if not current_user.can_manage_users():
        flash("Доступ запрещён", "error")
        return redirect(url_for("main.index"))

    new_role = request.form.get("role", "")
    try:
        change_user_role(uid, new_role, current_user.id)
        flash("Роль обновлена", "success")
    except UserError as exc:
        flash(exc.message, "error")

    return redirect(url_for("users.list_users"))
