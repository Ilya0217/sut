"""
Модуль: requirements_api.py
Назначение: REST API управления требованиями
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.Requirements, FR-01-FR-04, FR-07, FR-08
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required

from services.requirement_service import (
    RequirementError,
    create_requirement,
    edit_requirement,
    get_requirement_by_id,
    get_requirement_history,
    get_requirements_list,
    hard_delete_requirement,
    restore_requirement,
    search_requirements,
    soft_delete_requirement,
)
from services.traceability_service import check_integrity

req_api = Blueprint("requirements_api", __name__, url_prefix="/api/projects/<int:project_id>/requirements")


def req_to_dict(req):
    return {
        "id": req.id,
        "system_id": req.system_id,
        "custom_id": req.custom_id,
        "project_id": req.project_id,
        "title": req.title,
        "text": req.text,
        "category": req.category,
        "priority": req.priority,
        "status": req.status,
        "parent_id": req.parent_id,
        "responsible_user_id": req.responsible_user_id,
        "responsible_user": req.responsible_user.username if req.responsible_user else None,
        "version": req.version,
        "is_baseline": req.is_baseline,
        "is_deleted": req.is_deleted,
        "created_at": req.created_at.isoformat() if req.created_at else None,
        "updated_at": req.updated_at.isoformat() if req.updated_at else None,
        "created_by": req.creator.username if req.creator else None,
    }


def history_to_dict(h):
    return {
        "id": h.id,
        "event_type": h.event_type,
        "attribute_name": h.attribute_name,
        "old_value": h.old_value,
        "new_value": h.new_value,
        "user": h.user.username if h.user else None,
        "created_at": h.created_at.isoformat() if h.created_at else None,
    }


@req_api.route("", methods=["GET"])
@login_required
def list_reqs(project_id):
    """
    Назначение: Список требований с фильтрацией и поиском
    Id требования: LLR_RequirementService_View_01, Functional.SearchAndFilter
    """
    page = request.args.get("page", 1, type=int)
    per_page = request.args.get("per_page", 50, type=int)
    search_q = request.args.get("q", "").strip()
    sort_by = request.args.get("sort_by", "updated_at")
    sort_order = request.args.get("sort_order", "desc")
    include_deleted = request.args.get("include_deleted", "0") == "1"

    if search_q:
        pagination = search_requirements(project_id, search_q, page=page, per_page=per_page)
    else:
        filters = {}
        for key in ["status", "priority", "category", "responsible_user_id"]:
            val = request.args.get(key, "").strip()
            if val:
                filters[key] = val

        pagination = get_requirements_list(
            project_id,
            filters=filters if filters else None,
            sort_by=sort_by,
            sort_order=sort_order,
            page=page,
            per_page=per_page,
            include_deleted=include_deleted,
        )

    return jsonify({
        "items": [req_to_dict(r) for r in pagination.items],
        "total": pagination.total,
        "pages": pagination.pages,
        "page": pagination.page,
        "per_page": per_page,
    })


@req_api.route("", methods=["POST"])
@login_required
def create_req(project_id):
    """
    Назначение: Создание нового требования
    Id требования: LLR_RequirementService_Create_01-05
    """
    if not current_user.can_create():
        return jsonify({"error": "Недостаточно прав", "code": "ERR_FORBIDDEN"}), 403

    data = request.get_json(silent=True) or {}
    try:
        req = create_requirement(project_id, data, current_user.id)
        return jsonify(req_to_dict(req)), 201
    except RequirementError as exc:
        return jsonify({"error": exc.message, "code": exc.code, "field": exc.field}), 400


@req_api.route("/<int:req_id>", methods=["GET"])
@login_required
def get_req(project_id, req_id):
    """Назначение: Получение требования по ID."""
    req = get_requirement_by_id(req_id)
    if req is None:
        return jsonify({"error": "Не найдено"}), 404
    return jsonify(req_to_dict(req))


@req_api.route("/<int:req_id>", methods=["PUT"])
@login_required
def update_req(project_id, req_id):
    """
    Назначение: Редактирование требования
    Id требования: LLR_RequirementService_Edit_01-02
    """
    if not current_user.can_edit():
        return jsonify({"error": "Недостаточно прав", "code": "ERR_FORBIDDEN"}), 403

    data = request.get_json(silent=True) or {}
    try:
        req = edit_requirement(req_id, data, current_user.id)
        check_integrity(req_id, current_user.id)
        return jsonify(req_to_dict(req))
    except RequirementError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@req_api.route("/<int:req_id>", methods=["DELETE"])
@login_required
def delete_req(project_id, req_id):
    """
    Назначение: Мягкое удаление требования
    Id требования: LLR_RequirementService_Delete_01
    """
    if not current_user.can_delete():
        return jsonify({"error": "Недостаточно прав", "code": "ERR_FORBIDDEN"}), 403

    try:
        soft_delete_requirement(req_id, current_user.id)
        check_integrity(req_id, current_user.id)
        return jsonify({"message": "Удалено"})
    except RequirementError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@req_api.route("/<int:req_id>/hard_delete", methods=["DELETE"])
@login_required
def hard_delete_req(project_id, req_id):
    """
    Назначение: Безвозвратное удаление
    Id требования: LLR_RequirementService_Delete_02
    """
    try:
        hard_delete_requirement(req_id, current_user.id)
        return jsonify({"message": "Удалено безвозвратно"})
    except RequirementError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@req_api.route("/<int:req_id>/restore", methods=["POST"])
@login_required
def restore_req(project_id, req_id):
    """Назначение: Восстановление удалённого требования."""
    try:
        req = restore_requirement(req_id, current_user.id)
        return jsonify(req_to_dict(req))
    except RequirementError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@req_api.route("/<int:req_id>/history", methods=["GET"])
@login_required
def get_history(project_id, req_id):
    """
    Назначение: История изменений требования
    Id требования: Functional.RequirementManagement.EditAndHistory
    """
    page = request.args.get("page", 1, type=int)
    pagination = get_requirement_history(req_id, page=page)
    return jsonify({
        "items": [history_to_dict(h) for h in pagination.items],
        "total": pagination.total,
        "pages": pagination.pages,
        "page": pagination.page,
    })
