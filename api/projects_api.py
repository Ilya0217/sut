"""
Модуль: projects_api.py
Назначение: REST API управления проектами
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.Projects
"""

from flask import Blueprint, jsonify, request
from flask_login import current_user, login_required

from models import Project, db
from services.audit_service import log_action

projects_api = Blueprint("projects_api", __name__, url_prefix="/api/projects")


def project_to_dict(project):
    return {
        "id": project.id,
        "name": project.name,
        "description": project.description,
        "created_at": project.created_at.isoformat(),
        "created_by": project.created_by,
    }


@projects_api.route("", methods=["GET"])
@login_required
def list_projects():
    """Назначение: Получение списка проектов."""
    projects = Project.query.order_by(Project.created_at.desc()).all()
    return jsonify([project_to_dict(p) for p in projects])


@projects_api.route("", methods=["POST"])
@login_required
def create_project():
    """
    Назначение: Создание нового проекта
    Id требования: Interface.Software.Projects
    """
    data = request.get_json(silent=True) or {}
    name = data.get("name", "").strip()
    if not name:
        return jsonify({"error": "Название проекта обязательно"}), 400

    project = Project(
        name=name,
        description=data.get("description", "").strip(),
        created_by=current_user.id,
    )
    db.session.add(project)
    db.session.commit()

    log_action(
        user_id=current_user.id,
        action="create",
        object_type="project",
        object_id=project.id,
        new_value=name,
    )
    return jsonify(project_to_dict(project)), 201


@projects_api.route("/<int:project_id>", methods=["GET"])
@login_required
def get_project(project_id):
    """Назначение: Получение информации о проекте."""
    project = Project.query.get_or_404(project_id)
    return jsonify(project_to_dict(project))
