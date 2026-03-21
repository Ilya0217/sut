"""
Модуль: audit_service.py
Назначение: Сервис ведения журнала аудита
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Functional.Audit.Logging, LLR_SecurityManager_Audit_01
"""

from flask import request as flask_request

from models import AuditLog, db


def log_action(user_id, action, object_type, object_id=None,
               old_value=None, new_value=None, context=None):
    """
    Назначение: Запись действия в журнал аудита
    Id требования: LLR_SecurityManager_Audit_01
    Входные данные: user_id, action, object_type, object_id, old/new value, context
    Выходные данные: объект AuditLog
    """
    ip_address = None
    try:
        ip_address = flask_request.remote_addr
    except RuntimeError:
        pass

    entry = AuditLog(
        user_id=user_id,
        action=action,
        object_type=object_type,
        object_id=str(object_id) if object_id is not None else None,
        old_value=str(old_value) if old_value is not None else None,
        new_value=str(new_value) if new_value is not None else None,
        context=context,
        ip_address=ip_address,
    )
    db.session.add(entry)
    db.session.commit()
    return entry


def get_audit_logs(page=1, per_page=50, object_type=None, user_id=None):
    """
    Назначение: Получение записей журнала аудита с фильтрацией
    Id требования: Functional.Audit.Logging
    Входные данные: page, per_page, фильтры
    Выходные данные: пагинированный список записей
    """
    query = AuditLog.query.order_by(AuditLog.created_at.desc())

    if object_type is not None:
        query = query.filter(AuditLog.object_type == object_type)
    if user_id is not None:
        query = query.filter(AuditLog.user_id == user_id)

    return query.paginate(page=page, per_page=per_page, error_out=False)
