"""
Модуль: tracelink_routes.py
Назначение: Маршруты управления связями трассировки
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Interface.Software.TraceLinks, FR-05, FR-06
"""

from flask import Blueprint, flash, redirect, render_template, request, url_for
from flask_login import current_user, login_required

from models import Project, Requirement, TraceLink
from services.notification_service import get_unread_count
from services.traceability_service import (
    TraceLinkError,
    create_trace_link,
    delete_trace_link,
    get_links_for_requirement,
    run_full_integrity_check,
)

trace_bp = Blueprint("tracelinks", __name__)


@trace_bp.route("/projects/<int:project_id>/tracelinks")
@login_required
def list_tracelinks(project_id):
    """
    Назначение: Список связей трассировки проекта
    Id требования: Functional.Traceability.LinkManagement.Manage
    """
    project = Project.query.get_or_404(project_id)
    links = TraceLink.query.join(
        Requirement, TraceLink.source_req_id == Requirement.id
    ).filter(Requirement.project_id == project_id).order_by(
        TraceLink.created_at.desc()
    ).all()

    return render_template(
        "tracelinks.html",
        project=project,
        links=links,
        unread_count=get_unread_count(current_user.id),
    )


@trace_bp.route("/projects/<int:project_id>/tracelinks/new", methods=["GET", "POST"])
@login_required
def new_tracelink(project_id):
    """
    Назначение: Создание новой связи трассировки
    Id требования: Functional.Traceability.LinkManagement.Create
    """
    project = Project.query.get_or_404(project_id)

    if not current_user.can_manage_links():
        flash("Недостаточно прав для управления связями", "error")
        return redirect(url_for("tracelinks.list_tracelinks", project_id=project_id))

    if request.method == "POST":
        source_id = request.form.get("source_id", type=int)
        target_id = request.form.get("target_id", type=int)
        link_type = request.form.get("link_type", "derives_from")
        description = request.form.get("description", "")

        try:
            link = create_trace_link(
                source_id, target_id, link_type,
                current_user.id, description,
            )
            flash(f"Связь #{link.id} создана", "success")
            return redirect(url_for("tracelinks.list_tracelinks", project_id=project_id))
        except TraceLinkError as exc:
            flash(exc.message, "error")

    requirements = Requirement.query.filter_by(
        project_id=project_id, is_deleted=False
    ).order_by(Requirement.system_id).all()

    return render_template(
        "tracelink_form.html",
        project=project,
        requirements=requirements,
        link_types=TraceLink.VALID_TYPES,
        unread_count=get_unread_count(current_user.id),
    )


@trace_bp.route("/projects/<int:project_id>/tracelinks/<int:link_id>/delete",
                methods=["POST"])
@login_required
def delete_link(project_id, link_id):
    """
    Назначение: Удаление связи трассировки
    Id требования: Functional.Traceability.LinkManagement.Manage
    """
    if not current_user.can_manage_links():
        flash("Недостаточно прав", "error")
        return redirect(url_for("tracelinks.list_tracelinks", project_id=project_id))

    try:
        delete_trace_link(link_id, current_user.id)
        flash("Связь удалена", "success")
    except TraceLinkError as exc:
        flash(exc.message, "error")

    return redirect(url_for("tracelinks.list_tracelinks", project_id=project_id))


@trace_bp.route("/projects/<int:project_id>/integrity_check", methods=["POST"])
@login_required
def integrity_check(project_id):
    """
    Назначение: Полная проверка целостности связей проекта
    Id требования: Functional.Traceability.IntegrityCheck.OnDemand
    """
    if not current_user.can_run_integrity_check():
        flash("Недостаточно прав для запуска проверки", "error")
        return redirect(url_for("tracelinks.list_tracelinks", project_id=project_id))

    issues = run_full_integrity_check(project_id, current_user.id)
    if issues:
        flash(
            f"Проверка завершена. Обнаружено проблем: {len(issues)}",
            "warning",
        )
    else:
        flash("Проверка завершена. Нарушений целостности не обнаружено", "success")

    return redirect(url_for("tracelinks.list_tracelinks", project_id=project_id))
