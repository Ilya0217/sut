"""
Модуль: notification_routes.py
Назначение: Маршруты системы уведомлений
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Functional.NotificationSystem.Alerts
"""

from flask import Blueprint, flash, redirect, render_template, request, url_for
from flask_login import current_user, login_required

from services.notification_service import (
    get_unread_count,
    get_user_notifications,
    mark_as_read,
)

notif_bp = Blueprint("notifications", __name__)


@notif_bp.route("/notifications")
@login_required
def list_notifications():
    """Назначение: Список уведомлений пользователя."""
    page = request.args.get("page", 1, type=int)
    unread_only = request.args.get("unread", "0") == "1"
    pagination = get_user_notifications(
        current_user.id, unread_only=unread_only, page=page,
    )
    return render_template(
        "notifications.html",
        pagination=pagination,
        notifications=pagination.items,
        unread_only=unread_only,
        unread_count=get_unread_count(current_user.id),
    )


@notif_bp.route("/notifications/<int:notif_id>/read", methods=["POST"])
@login_required
def read_notification(notif_id):
    """Назначение: Пометить уведомление как прочитанное."""
    mark_as_read(notif_id, current_user.id)
    return redirect(url_for("notifications.list_notifications"))
