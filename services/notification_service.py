"""
Модуль: notification_service.py
Назначение: Сервис уведомлений пользователей
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Functional.NotificationSystem.Alerts,
             LLR_NotificationService_ProcessEvent_01,
             LLR_NotificationService_DeliverAlert_01-02
"""

from models import Notification, db
from services.audit_service import log_action


def create_notification(user_id, event_type, message,
                        related_object_type=None, related_object_id=None):
    """
    Назначение: Создание уведомления и помещение в очередь
    Id требования: LLR_NotificationService_DeliverAlert_01
    Входные данные: user_id, event_type, message, связанный объект
    Выходные данные: объект Notification
    """
    notification = Notification(
        user_id=user_id,
        event_type=event_type,
        message=message,
        related_object_type=related_object_type,
        related_object_id=related_object_id,
    )
    db.session.add(notification)
    db.session.commit()

    log_action(
        user_id=None,
        action="notification_sent",
        object_type="notification",
        object_id=notification.id,
        context=f"type={event_type}, recipient={user_id}",
    )
    return notification


def notify_integrity_failure(link_id, requirement_id, responsible_user_id, reason):
    """
    Назначение: Уведомление о нарушении целостности связи
    Id требования: LLR_NotificationService_ProcessEvent_01
    Входные данные: link_id, requirement_id, responsible_user_id, reason
    Выходные данные: объект Notification
    """
    message = (
        f"Обнаружено нарушение целостности связи #{link_id}. "
        f"Требование #{requirement_id}: {reason}. "
        f"Требуется проверка."
    )
    return create_notification(
        user_id=responsible_user_id,
        event_type="LINK_INTEGRITY_FAILED",
        message=message,
        related_object_type="trace_link",
        related_object_id=link_id,
    )


def get_user_notifications(user_id, unread_only=False, page=1, per_page=20):
    """
    Назначение: Получение уведомлений пользователя
    Id требования: Functional.NotificationSystem.Alerts
    Входные данные: user_id, фильтры
    Выходные данные: пагинированный список уведомлений
    """
    query = Notification.query.filter_by(user_id=user_id)
    if unread_only:
        query = query.filter_by(is_read=False)
    query = query.order_by(Notification.created_at.desc())
    return query.paginate(page=page, per_page=per_page, error_out=False)


def mark_as_read(notification_id, user_id):
    """
    Назначение: Пометить уведомление как прочитанное
    Id требования: Functional.NotificationSystem.Alerts
    Входные данные: notification_id, user_id
    Выходные данные: True/False
    """
    notification = Notification.query.filter_by(
        id=notification_id, user_id=user_id
    ).first()
    if notification is None:
        return False
    notification.is_read = True
    db.session.commit()
    return True


def get_unread_count(user_id):
    """
    Назначение: Подсчет непрочитанных уведомлений
    Входные данные: user_id
    Выходные данные: целое число
    """
    return Notification.query.filter_by(
        user_id=user_id, is_read=False
    ).count()
