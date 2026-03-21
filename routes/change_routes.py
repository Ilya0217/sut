"""
Модуль: change_routes.py
Назначение: Маршруты управления изменениями и базовыми версиями
Автор: Разработчик
Дата создания: 20.03.2026
Требования: FR-11, Functional.ConfigurationManagement.BaselineAndChange
"""

from flask import Blueprint, flash, redirect, render_template, request, url_for
from flask_login import current_user, login_required

from models import Baseline, ChangeRequest, Project, Requirement, User
from services.change_management_service import (
    ChangeError,
    approve_change_request,
    create_baseline,
    create_change_request,
    get_change_requests,
    reject_change_request,
)
from services.notification_service import get_unread_count

change_bp = Blueprint("changes", __name__)


@change_bp.route("/projects/<int:project_id>/baselines")
@login_required
def list_baselines(project_id):
    """Назначение: Список базовых версий проекта."""
    project = Project.query.get_or_404(project_id)
    baselines = Baseline.query.filter_by(project_id=project_id).order_by(
        Baseline.created_at.desc()
    ).all()
    return render_template(
        "baselines.html",
        project=project,
        baselines=baselines,
        unread_count=get_unread_count(current_user.id),
    )


@change_bp.route("/projects/<int:project_id>/baselines/new", methods=["GET", "POST"])
@login_required
def new_baseline(project_id):
    """
    Назначение: Создание базовой версии
    Id требования: LLR_BaselineManager_CreateBaseline_01
    """
    project = Project.query.get_or_404(project_id)

    if request.method == "POST":
        name = request.form.get("name", "").strip()
        description = request.form.get("description", "").strip()
        req_ids = request.form.getlist("requirement_ids", type=int)

        if not name:
            flash("Название базовой версии обязательно", "error")
        elif not req_ids:
            flash("Выберите хотя бы одно требование", "error")
        else:
            baseline = create_baseline(
                project_id, name, req_ids, current_user.id, description,
            )
            flash(f"Базовая версия '{name}' создана", "success")
            return redirect(
                url_for("changes.list_baselines", project_id=project_id)
            )

    requirements = Requirement.query.filter_by(
        project_id=project_id, is_deleted=False, is_baseline=False,
    ).order_by(Requirement.system_id).all()

    return render_template(
        "baseline_form.html",
        project=project,
        requirements=requirements,
        unread_count=get_unread_count(current_user.id),
    )


@change_bp.route("/projects/<int:project_id>/change_requests")
@login_required
def list_change_requests(project_id):
    """Назначение: Список запросов на изменение проекта."""
    project = Project.query.get_or_404(project_id)
    page = request.args.get("page", 1, type=int)
    status_filter = request.args.get("status", "").strip()

    pagination = get_change_requests(
        project_id=project_id,
        status=status_filter if status_filter else None,
        page=page,
    )
    return render_template(
        "change_requests.html",
        project=project,
        pagination=pagination,
        change_requests=pagination.items,
        status_filter=status_filter,
        unread_count=get_unread_count(current_user.id),
    )


@change_bp.route("/projects/<int:project_id>/change_requests/new",
                  methods=["GET", "POST"])
@login_required
def new_change_request(project_id):
    """
    Назначение: Создание запроса на изменение
    Id требования: LLR_BaselineManager_ProcessChange_01
    """
    project = Project.query.get_or_404(project_id)

    if request.method == "POST":
        req_id = request.form.get("requirement_id", type=int)
        justification = request.form.get("justification", "").strip()
        changes_desc = request.form.get("changes_description", "").strip()
        assigned_to = request.form.get("assigned_to", type=int)

        if not req_id or not justification or not changes_desc:
            flash("Все поля обязательны", "error")
        else:
            try:
                cr = create_change_request(
                    req_id, current_user.id, justification,
                    changes_desc, assigned_to,
                )
                flash(f"Запрос на изменение #{cr.id} создан", "success")
                return redirect(
                    url_for("changes.list_change_requests", project_id=project_id)
                )
            except ChangeError as exc:
                flash(exc.message, "error")

    requirements = Requirement.query.filter_by(
        project_id=project_id, is_deleted=False, is_baseline=True,
    ).order_by(Requirement.system_id).all()
    users = User.query.filter(
        User.role.in_(["reviewer", "admin"])
    ).order_by(User.username).all()

    return render_template(
        "change_request_form.html",
        project=project,
        requirements=requirements,
        users=users,
        unread_count=get_unread_count(current_user.id),
    )


@change_bp.route("/projects/<int:project_id>/change_requests/<int:cr_id>/approve",
                  methods=["POST"])
@login_required
def approve_cr(project_id, cr_id):
    """Назначение: Одобрение запроса на изменение."""
    if not current_user.can_approve_changes():
        flash("Недостаточно прав для согласования", "error")
        return redirect(
            url_for("changes.list_change_requests", project_id=project_id)
        )

    comment = request.form.get("comment", "")
    try:
        approve_change_request(cr_id, current_user.id, comment)
        flash("Запрос одобрен", "success")
    except ChangeError as exc:
        flash(exc.message, "error")

    return redirect(url_for("changes.list_change_requests", project_id=project_id))


@change_bp.route("/projects/<int:project_id>/change_requests/<int:cr_id>/reject",
                  methods=["POST"])
@login_required
def reject_cr(project_id, cr_id):
    """Назначение: Отклонение запроса на изменение."""
    if not current_user.can_approve_changes():
        flash("Недостаточно прав для отклонения", "error")
        return redirect(
            url_for("changes.list_change_requests", project_id=project_id)
        )

    comment = request.form.get("comment", "")
    try:
        reject_change_request(cr_id, current_user.id, comment)
        flash("Запрос отклонён", "success")
    except ChangeError as exc:
        flash(exc.message, "error")

    return redirect(url_for("changes.list_change_requests", project_id=project_id))
