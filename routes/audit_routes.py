"""
Модуль: audit_routes.py
Назначение: Маршруты журнала аудита
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Functional.Audit.Logging
"""

from flask import Blueprint, render_template, request
from flask_login import current_user, login_required

from services.audit_service import get_audit_logs
from services.notification_service import get_unread_count

audit_bp = Blueprint("audit", __name__)


@audit_bp.route("/audit")
@login_required
def audit_log():
    """Назначение: Просмотр журнала аудита."""
    page = request.args.get("page", 1, type=int)
    object_type = request.args.get("object_type", "").strip()
    user_id = request.args.get("user_id", type=int)

    pagination = get_audit_logs(
        page=page,
        object_type=object_type if object_type else None,
        user_id=user_id,
    )
    return render_template(
        "audit_log.html",
        pagination=pagination,
        logs=pagination.items,
        object_type_filter=object_type,
        user_id_filter=user_id,
        unread_count=get_unread_count(current_user.id),
    )
