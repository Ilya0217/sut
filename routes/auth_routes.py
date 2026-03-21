"""
Модуль: auth_routes.py
Назначение: Маршруты аутентификации и авторизации
Автор: Разработчик
Дата создания: 20.03.2026
Требования: LLR_SecurityManager_Authenticate_01, Functional.Security.UserManagement
"""

from flask import Blueprint, flash, redirect, render_template, request, url_for
from flask_login import login_required, login_user, logout_user

from services.audit_service import log_action
from services.user_service import UserError, authenticate_user, register_user

auth_bp = Blueprint("auth", __name__)


@auth_bp.route("/login", methods=["GET", "POST"])
def login():
    """
    Назначение: Страница входа в систему
    Id требования: LLR_SecurityManager_Authenticate_01
    """
    if request.method == "POST":
        username = request.form.get("username", "").strip()
        password = request.form.get("password", "")

        user = authenticate_user(username, password)
        if user is not None:
            login_user(user)
            next_page = request.args.get("next")
            return redirect(next_page or url_for("main.index"))

        log_action(
            user_id=None,
            action="failed_login_attempt",
            object_type="auth",
            new_value=username,
        )
        flash("Неверное имя пользователя или пароль", "error")

    return render_template("login.html")


@auth_bp.route("/register", methods=["GET", "POST"])
def register():
    """
    Назначение: Страница регистрации
    Id требования: Functional.Security.UserManagement
    """
    if request.method == "POST":
        username = request.form.get("username", "").strip()
        email = request.form.get("email", "").strip()
        password = request.form.get("password", "")
        role = request.form.get("role", "analyst")

        try:
            user = register_user(username, email, password, role)
            login_user(user)
            flash("Регистрация успешна", "success")
            return redirect(url_for("main.index"))
        except UserError as exc:
            flash(exc.message, "error")

    return render_template("register.html")


@auth_bp.route("/logout")
@login_required
def logout():
    """Назначение: Выход из системы."""
    logout_user()
    return redirect(url_for("auth.login"))
