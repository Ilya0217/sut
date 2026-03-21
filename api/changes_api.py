"""
Модуль: changes_api.py
Назначение: REST API управления изменениями и базовыми версиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: FR-11, Functional.ConfigurationManagement.BaselineAndChange
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required

from models import Baseline, ChangeRequest, Requirement
from services.change_management_service import (
    ChangeError,
    approve_change_request,
    create_baseline,
    create_change_request,
    get_change_requests,
    reject_change_request,
)

changes_api = Blueprint("changes_api", __name__, url_prefix="/api/projects/<int:project_id>")


def baseline_to_dict(bl):
    return {
        "id": bl.id,
        "name": bl.name,
        "description": bl.description,
        "created_at": bl.created_at.isoformat() if bl.created_at else None,
        "requirements_count": bl.requirements.count(),
    }


def cr_to_dict(cr):
    return {
        "id": cr.id,
        "requirement_id": cr.requirement_id,
        "requirement_system_id": cr.requirement.system_id if cr.requirement else None,
        "status": cr.status,
        "justification": cr.justification,
        "changes_description": cr.changes_description,
        "requested_by": cr.requester.username if cr.requester else None,
        "assigned_to": cr.assignee.username if cr.assignee else None,
        "created_at": cr.created_at.isoformat() if cr.created_at else None,
        "resolved_at": cr.resolved_at.isoformat() if cr.resolved_at else None,
        "resolution_comment": cr.resolution_comment,
    }


@changes_api.route("/baselines", methods=["GET"])
@login_required
def list_baselines(project_id):
    """Назначение: Список базовых версий."""
    baselines = Baseline.query.filter_by(project_id=project_id).order_by(
        Baseline.created_at.desc()
    ).all()
    return jsonify([baseline_to_dict(bl) for bl in baselines])


@changes_api.route("/baselines", methods=["POST"])
@login_required
def new_baseline(project_id):
    """
    Назначение: Создание базовой версии
    Id требования: LLR_BaselineManager_CreateBaseline_01
    """
    data = request.get_json(silent=True) or {}
    name = data.get("name", "").strip()
    req_ids = data.get("requirement_ids", [])

    if not name:
        return jsonify({"error": "Название обязательно"}), 400
    if not req_ids:
        return jsonify({"error": "Выберите хотя бы одно требование"}), 400

    bl = create_baseline(
        project_id, name, req_ids, current_user.id,
        data.get("description", ""),
    )
    return jsonify(baseline_to_dict(bl)), 201


@changes_api.route("/change_requests", methods=["GET"])
@login_required
def list_crs(project_id):
    """Назначение: Список запросов на изменение."""
    page = request.args.get("page", 1, type=int)
    status = request.args.get("status", "").strip() or None
    pagination = get_change_requests(project_id=project_id, status=status, page=page)
    return jsonify({
        "items": [cr_to_dict(cr) for cr in pagination.items],
        "total": pagination.total,
        "pages": pagination.pages,
        "page": pagination.page,
    })


@changes_api.route("/change_requests", methods=["POST"])
@login_required
def new_cr(project_id):
    """
    Назначение: Создание запроса на изменение
    Id требования: LLR_BaselineManager_ProcessChange_01
    """
    data = request.get_json(silent=True) or {}
    try:
        cr = create_change_request(
            requirement_id=data.get("requirement_id"),
            user_id=current_user.id,
            justification=data.get("justification", ""),
            changes_description=data.get("changes_description", ""),
            assigned_to=data.get("assigned_to"),
        )
        return jsonify(cr_to_dict(cr)), 201
    except ChangeError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@changes_api.route("/change_requests/<int:cr_id>/approve", methods=["POST"])
@login_required
def approve_cr(project_id, cr_id):
    """Назначение: Одобрение запроса на изменение."""
    if not current_user.can_approve_changes():
        return jsonify({"error": "Недостаточно прав"}), 403

    data = request.get_json(silent=True) or {}
    try:
        cr = approve_change_request(cr_id, current_user.id, data.get("comment", ""))
        return jsonify(cr_to_dict(cr))
    except ChangeError as exc:
        return jsonify({"error": exc.message}), 400


@changes_api.route("/change_requests/<int:cr_id>/reject", methods=["POST"])
@login_required
def reject_cr(project_id, cr_id):
    """Назначение: Отклонение запроса на изменение."""
    if not current_user.can_approve_changes():
        return jsonify({"error": "Недостаточно прав"}), 403

    data = request.get_json(silent=True) or {}
    try:
        cr = reject_change_request(cr_id, current_user.id, data.get("comment", ""))
        return jsonify(cr_to_dict(cr))
    except ChangeError as exc:
        return jsonify({"error": exc.message}), 400
