"""
Модуль: main_routes.py
Назначение: Главные маршруты приложения (проекты, дашборд)
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Interface.Software.Projects
"""

from flask import Blueprint, flash, redirect, render_template, request, url_for
from flask_login import current_user, login_required

from models import Project, db
from services.audit_service import log_action
from services.notification_service import get_unread_count

main_bp = Blueprint("main", __name__)


@main_bp.route("/")
@login_required
def index():
    """Назначение: Главная страница — список проектов."""
    projects = Project.query.order_by(Project.created_at.desc()).all()
    unread = get_unread_count(current_user.id)
    return render_template("index.html", projects=projects, unread_count=unread)


@main_bp.route("/projects/new", methods=["GET", "POST"])
@login_required
def create_project():
    """
    Назначение: Создание нового проекта
    Id требования: Interface.Software.Projects
    """
    if request.method == "POST":
        name = request.form.get("name", "").strip()
        description = request.form.get("description", "").strip()

        if not name:
            flash("Название проекта обязательно", "error")
            return render_template("project_form.html")

        project = Project(
            name=name,
            description=description,
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
        flash("Проект создан", "success")
        return redirect(url_for("requirements.list_requirements", project_id=project.id))

    return render_template("project_form.html")
