"""
Модуль: audit_api.py
Назначение: REST API журнала аудита
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Functional.Audit.Logging
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required

from services.audit_service import get_audit_logs

audit_api = Blueprint("audit_api", __name__, url_prefix="/api/audit")


@audit_api.route("", methods=["GET"])
@login_required
def list_logs():
    """Назначение: Получение записей журнала аудита."""
    page = request.args.get("page", 1, type=int)
    per_page = request.args.get("per_page", 50, type=int)
    object_type = request.args.get("object_type", "").strip() or None
    user_id = request.args.get("user_id", type=int)

    pagination = get_audit_logs(
        page=page, per_page=per_page,
        object_type=object_type, user_id=user_id,
    )
    return jsonify({
        "items": [
            {
                "id": log.id,
                "user": log.user.username if log.user else None,
                "action": log.action,
                "object_type": log.object_type,
                "object_id": log.object_id,
                "old_value": log.old_value,
                "new_value": log.new_value,
                "context": log.context,
                "ip_address": log.ip_address,
                "created_at": log.created_at.isoformat() if log.created_at else None,
            }
            for log in pagination.items
        ],
        "total": pagination.total,
        "pages": pagination.pages,
        "page": pagination.page,
    })
