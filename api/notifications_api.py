"""
Модуль: notifications_api.py
Назначение: REST API уведомлений
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.NotificationSystem.Alerts
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required

from services.notification_service import (
    get_unread_count,
    get_user_notifications,
    mark_as_read,
)

notif_api = Blueprint("notifications_api", __name__, url_prefix="/api/notifications")


@notif_api.route("", methods=["GET"])
@login_required
def list_notifications():
    """Назначение: Список уведомлений."""
    page = request.args.get("page", 1, type=int)
    unread_only = request.args.get("unread", "0") == "1"
    pagination = get_user_notifications(current_user.id, unread_only=unread_only, page=page)
    return jsonify({
        "items": [
            {
                "id": n.id,
                "event_type": n.event_type,
                "message": n.message,
                "is_read": n.is_read,
                "related_object_type": n.related_object_type,
                "related_object_id": n.related_object_id,
                "created_at": n.created_at.isoformat() if n.created_at else None,
            }
            for n in pagination.items
        ],
        "total": pagination.total,
        "pages": pagination.pages,
        "page": pagination.page,
        "unread_count": get_unread_count(current_user.id),
    })


@notif_api.route("/<int:notif_id>/read", methods=["POST"])
@login_required
def read_notif(notif_id):
    """Назначение: Пометить как прочитанное."""
    mark_as_read(notif_id, current_user.id)
    return jsonify({"message": "OK"})


@notif_api.route("/unread_count", methods=["GET"])
@login_required
def unread_count():
    """Назначение: Количество непрочитанных."""
    return jsonify({"count": get_unread_count(current_user.id)})
