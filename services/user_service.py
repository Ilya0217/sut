"""
Модуль: user_service.py
Назначение: Управление пользователями и правами доступа
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Functional.Security.UserManagement, Functional.Security.AccessControl,
             LLR_SecurityManager_Authenticate_01, LLR_SecurityManager_Authorize_01
"""

from models import User, db
from services.audit_service import log_action


class UserError(Exception):
    """Базовое исключение для ошибок работы с пользователями."""

    def __init__(self, code, message):
        self.code = code
        self.message = message
        super().__init__(message)


def register_user(username, email, password, role="analyst"):
    """
    Назначение: Регистрация нового пользователя
    Id требования: Functional.Security.UserManagement
    Входные данные: username, email, password, role
    Выходные данные: объект User или исключение
    """
    if not username or not email or not password:
        raise UserError("ERR_MISSING_FIELD", "Все поля обязательны для заполнения")

    if role not in User.VALID_ROLES:
        raise UserError("ERR_INVALID_ROLE", f"Недопустимая роль: {role}")

    if User.query.filter_by(username=username).first() is not None:
        raise UserError("ERR_DUPLICATE_USERNAME", "Имя пользователя уже занято")

    if User.query.filter_by(email=email).first() is not None:
        raise UserError("ERR_DUPLICATE_EMAIL", "Email уже используется")

    user = User(username=username, email=email, role=role)
    user.set_password(password)
    db.session.add(user)
    db.session.commit()

    log_action(
        user_id=user.id,
        action="register",
        object_type="user",
        object_id=user.id,
        new_value=f"{username} ({role})",
    )
    return user


def authenticate_user(username, password):
    """
    Назначение: Аутентификация пользователя
    Id требования: LLR_SecurityManager_Authenticate_01
    Входные данные: username, password
    Выходные данные: объект User или None
    """
    user = User.query.filter_by(username=username).first()
    if user is None or not user.check_password(password):
        log_action(
            user_id=None,
            action="failed_login",
            object_type="user",
            new_value=username,
        )
        return None

    log_action(
        user_id=user.id,
        action="login",
        object_type="user",
        object_id=user.id,
    )
    return user


def change_user_role(user_id, new_role, admin_id):
    """
    Назначение: Изменение роли пользователя
    Id требования: Functional.Security.UserManagement
    Входные данные: user_id, new_role, admin_id
    Выходные данные: объект User или исключение
    """
    if new_role not in User.VALID_ROLES:
        raise UserError("ERR_INVALID_ROLE", f"Недопустимая роль: {new_role}")

    user = User.query.filter_by(id=user_id).first()
    if user is None:
        raise UserError("ERR_USER_NOT_FOUND", "Пользователь не найден")

    old_role = user.role
    user.role = new_role
    db.session.commit()

    log_action(
        user_id=admin_id,
        action="change_role",
        object_type="user",
        object_id=user.id,
        old_value=old_role,
        new_value=new_role,
    )
    return user


def get_all_users():
    """
    Назначение: Получение списка всех пользователей
    Входные данные: нет
    Выходные данные: список объектов User
    """
    return User.query.order_by(User.username).all()
