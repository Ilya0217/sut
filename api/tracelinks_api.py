"""
Модуль: tracelinks_api.py
Назначение: REST API управления связями трассировки
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.TraceLinks, FR-05, FR-06, FR-12, FR-14
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required

from models import Requirement, TraceLink
from services.traceability_service import (
    TraceLinkError,
    create_trace_link,
    delete_trace_link,
    get_links_for_requirement,
    run_full_integrity_check,
)

trace_api = Blueprint("tracelinks_api", __name__, url_prefix="/api/projects/<int:project_id>/tracelinks")


def link_to_dict(link):
    return {
        "id": link.id,
        "source_req_id": link.source_req_id,
        "source_system_id": link.source_req.system_id if link.source_req else None,
        "source_title": link.source_req.title if link.source_req else None,
        "target_req_id": link.target_req_id,
        "target_system_id": link.target_req.system_id if link.target_req else None,
        "target_title": link.target_req.title if link.target_req else None,
        "link_type": link.link_type,
        "description": link.description,
        "status": link.status,
        "created_at": link.created_at.isoformat() if link.created_at else None,
        "created_by": link.creator.username if link.creator else None,
    }


@trace_api.route("", methods=["GET"])
@login_required
def list_links(project_id):
    """Назначение: Список связей трассировки проекта."""
    links = TraceLink.query.join(
        Requirement, TraceLink.source_req_id == Requirement.id
    ).filter(
        Requirement.project_id == project_id
    ).order_by(TraceLink.created_at.desc()).all()

    return jsonify([link_to_dict(lnk) for lnk in links])


@trace_api.route("", methods=["POST"])
@login_required
def create_link(project_id):
    """
    Назначение: Создание связи трассировки
    Id требования: LLR_TraceLinkService_CreateLink_01-02
    """
    if not current_user.can_manage_links():
        return jsonify({"error": "Недостаточно прав"}), 403

    data = request.get_json(silent=True) or {}
    try:
        link = create_trace_link(
            source_id=data.get("source_req_id"),
            target_id=data.get("target_req_id"),
            link_type=data.get("link_type", "derives_from"),
            user_id=current_user.id,
            description=data.get("description", ""),
        )
        return jsonify(link_to_dict(link)), 201
    except TraceLinkError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@trace_api.route("/<int:link_id>", methods=["DELETE"])
@login_required
def delete_link(project_id, link_id):
    """Назначение: Удаление связи трассировки."""
    if not current_user.can_manage_links():
        return jsonify({"error": "Недостаточно прав"}), 403

    try:
        delete_trace_link(link_id, current_user.id)
        return jsonify({"message": "Удалено"})
    except TraceLinkError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400


@trace_api.route("/requirement/<int:req_id>", methods=["GET"])
@login_required
def links_for_req(project_id, req_id):
    """Назначение: Связи для конкретного требования."""
    links = get_links_for_requirement(req_id)
    return jsonify([link_to_dict(lnk) for lnk in links])


@trace_api.route("/integrity_check", methods=["POST"])
@login_required
def integrity_check(project_id):
    """
    Назначение: Полная проверка целостности
    Id требования: Functional.Traceability.IntegrityCheck.OnDemand
    """
    if not current_user.can_run_integrity_check():
        return jsonify({"error": "Недостаточно прав"}), 403

    issues = run_full_integrity_check(project_id, current_user.id)
    return jsonify({
        "issues_count": len(issues),
        "issues": [
            {
                "link_id": item["link"].id,
                "reason": item["reason"],
            }
            for item in issues
        ],
    })
