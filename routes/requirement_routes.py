"""
Модуль: requirement_routes.py
Назначение: Маршруты управления требованиями
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Interface.Software.Requirements, FR-01-FR-04, FR-07, FR-08
"""

from flask import Blueprint, flash, redirect, render_template, request, url_for
from flask_login import current_user, login_required

from models import Project, Requirement, User
from services.notification_service import get_unread_count
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
from services.traceability_service import check_integrity, get_links_for_requirement

req_bp = Blueprint("requirements", __name__)


@req_bp.route("/projects/<int:project_id>/requirements")
@login_required
def list_requirements(project_id):
    """
    Назначение: Список требований проекта с фильтрацией
    Id требования: Functional.RequirementManagement.View, Functional.SearchAndFilter.Filtering
    """
    project = Project.query.get_or_404(project_id)
    page = request.args.get("page", 1, type=int)
    search_q = request.args.get("q", "").strip()

    filters = {}
    for key in ["status", "priority", "category"]:
        val = request.args.get(key, "").strip()
        if val:
            filters[key] = val

    sort_by = request.args.get("sort_by", "updated_at")
    sort_order = request.args.get("sort_order", "desc")

    if search_q:
        pagination = search_requirements(project_id, search_q, page=page)
    else:
        pagination = get_requirements_list(
            project_id, filters=filters if filters else None,
            sort_by=sort_by, sort_order=sort_order, page=page,
        )

    unread = get_unread_count(current_user.id)
    return render_template(
        "requirements.html",
        project=project,
        pagination=pagination,
        requirements=pagination.items,
        filters=filters,
        search_q=search_q,
        sort_by=sort_by,
        sort_order=sort_order,
        unread_count=unread,
    )


@req_bp.route("/projects/<int:project_id>/requirements/new", methods=["GET", "POST"])
@login_required
def new_requirement(project_id):
    """
    Назначение: Создание нового требования
    Id требования: Functional.RequirementManagement.Create
    """
    project = Project.query.get_or_404(project_id)

    if not current_user.can_create():
        flash("Недостаточно прав для создания требований", "error")
        return redirect(url_for("requirements.list_requirements", project_id=project_id))

    if request.method == "POST":
        user_data = {
            "title": request.form.get("title", ""),
            "text": request.form.get("text", ""),
            "category": request.form.get("category", "functional"),
            "priority": request.form.get("priority", "medium"),
            "status": request.form.get("status", "draft"),
            "custom_id": request.form.get("custom_id", ""),
            "parent_id": request.form.get("parent_id") or None,
            "responsible_user_id": request.form.get("responsible_user_id") or None,
        }
        if user_data["parent_id"]:
            user_data["parent_id"] = int(user_data["parent_id"])
        if user_data["responsible_user_id"]:
            user_data["responsible_user_id"] = int(user_data["responsible_user_id"])

        try:
            req = create_requirement(project_id, user_data, current_user.id)
            flash(f"Требование {req.system_id} создано", "success")
            return redirect(
                url_for("requirements.view_requirement",
                        project_id=project_id, req_id=req.id)
            )
        except RequirementError as exc:
            flash(exc.message, "error")

    users = User.query.order_by(User.username).all()
    parent_reqs = Requirement.query.filter_by(
        project_id=project_id, is_deleted=False
    ).order_by(Requirement.system_id).all()

    return render_template(
        "requirement_form.html",
        project=project,
        requirement=None,
        users=users,
        parent_reqs=parent_reqs,
        unread_count=get_unread_count(current_user.id),
    )


@req_bp.route("/projects/<int:project_id>/requirements/<int:req_id>")
@login_required
def view_requirement(project_id, req_id):
    """
    Назначение: Просмотр детальной информации о требовании
    Id требования: Functional.RequirementManagement.View
    """
    project = Project.query.get_or_404(project_id)
    requirement = get_requirement_by_id(req_id)
    if requirement is None:
        flash("Требование не найдено", "error")
        return redirect(url_for("requirements.list_requirements", project_id=project_id))

    links = get_links_for_requirement(req_id)
    history = get_requirement_history(req_id)
    children = Requirement.query.filter_by(
        parent_id=req_id, is_deleted=False
    ).all()

    return render_template(
        "requirement_detail.html",
        project=project,
        requirement=requirement,
        links=links,
        history=history.items,
        children=children,
        unread_count=get_unread_count(current_user.id),
    )


@req_bp.route("/projects/<int:project_id>/requirements/<int:req_id>/edit",
              methods=["GET", "POST"])
@login_required
def edit_req(project_id, req_id):
    """
    Назначение: Редактирование требования
    Id требования: Functional.RequirementManagement.EditAndHistory
    """
    project = Project.query.get_or_404(project_id)
    requirement = get_requirement_by_id(req_id)

    if not current_user.can_edit():
        flash("Недостаточно прав для редактирования", "error")
        return redirect(
            url_for("requirements.view_requirement",
                    project_id=project_id, req_id=req_id)
        )

    if request.method == "POST":
        user_data = {
            "title": request.form.get("title", ""),
            "text": request.form.get("text", ""),
            "category": request.form.get("category", ""),
            "priority": request.form.get("priority", ""),
            "status": request.form.get("status", ""),
            "custom_id": request.form.get("custom_id", ""),
            "parent_id": request.form.get("parent_id") or None,
            "responsible_user_id": request.form.get("responsible_user_id") or None,
        }
        if user_data["parent_id"]:
            user_data["parent_id"] = int(user_data["parent_id"])
        if user_data["responsible_user_id"]:
            user_data["responsible_user_id"] = int(user_data["responsible_user_id"])

        try:
            edit_requirement(req_id, user_data, current_user.id)
            check_integrity(req_id, current_user.id)
            flash("Требование обновлено", "success")
            return redirect(
                url_for("requirements.view_requirement",
                        project_id=project_id, req_id=req_id)
            )
        except RequirementError as exc:
            flash(exc.message, "error")

    users = User.query.order_by(User.username).all()
    parent_reqs = Requirement.query.filter(
        Requirement.project_id == project_id,
        Requirement.is_deleted == False,
        Requirement.id != req_id,
    ).order_by(Requirement.system_id).all()

    return render_template(
        "requirement_form.html",
        project=project,
        requirement=requirement,
        users=users,
        parent_reqs=parent_reqs,
        unread_count=get_unread_count(current_user.id),
    )


@req_bp.route("/projects/<int:project_id>/requirements/<int:req_id>/delete",
              methods=["POST"])
@login_required
def delete_req(project_id, req_id):
    """
    Назначение: Удаление требования (мягкое)
    Id требования: LLR_RequirementService_Delete_01
    """
    if not current_user.can_delete():
        flash("Недостаточно прав для удаления", "error")
        return redirect(
            url_for("requirements.view_requirement",
                    project_id=project_id, req_id=req_id)
        )

    try:
        soft_delete_requirement(req_id, current_user.id)
        check_integrity(req_id, current_user.id)
        flash("Требование удалено (можно восстановить)", "success")
    except RequirementError as exc:
        flash(exc.message, "error")

    return redirect(url_for("requirements.list_requirements", project_id=project_id))


@req_bp.route("/projects/<int:project_id>/requirements/<int:req_id>/hard_delete",
              methods=["POST"])
@login_required
def hard_delete_req(project_id, req_id):
    """
    Назначение: Безвозвратное удаление требования
    Id требования: LLR_RequirementService_Delete_02
    """
    try:
        hard_delete_requirement(req_id, current_user.id)
        flash("Требование удалено безвозвратно", "success")
    except RequirementError as exc:
        flash(exc.message, "error")

    return redirect(url_for("requirements.list_requirements", project_id=project_id))


@req_bp.route("/projects/<int:project_id>/requirements/<int:req_id>/restore",
              methods=["POST"])
@login_required
def restore_req(project_id, req_id):
    """
    Назначение: Восстановление удалённого требования
    Id требования: Functional.RequirementManagement.Delete
    """
    try:
        restore_requirement(req_id, current_user.id)
        flash("Требование восстановлено", "success")
    except RequirementError as exc:
        flash(exc.message, "error")

    return redirect(
        url_for("requirements.view_requirement",
                project_id=project_id, req_id=req_id)
    )
