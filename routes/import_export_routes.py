"""
Модуль: import_export_routes.py
Назначение: Маршруты импорта и экспорта данных
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Interface.Software.ImportExport, FR-09, FR-10
"""

from flask import (
    Blueprint, Response, flash, redirect, render_template,
    request, send_file, url_for,
)
from flask_login import current_user, login_required

from models import Project
from services.import_export_service import (
    ImportExportError,
    export_to_csv,
    export_to_json,
    export_to_xlsx,
    import_from_csv,
    import_from_json,
    import_from_xlsx,
    validate_xlsx_structure,
)
from services.notification_service import get_unread_count

ie_bp = Blueprint("import_export", __name__)


@ie_bp.route("/projects/<int:project_id>/import", methods=["GET", "POST"])
@login_required
def import_data(project_id):
    """
    Назначение: Импорт данных из файла
    Id требования: Functional.ImportExport.Import
    """
    project = Project.query.get_or_404(project_id)

    if not current_user.can_import_export():
        flash("Недостаточно прав для импорта", "error")
        return redirect(url_for("requirements.list_requirements", project_id=project_id))

    if request.method == "POST":
        file = request.files.get("file")
        if file is None or file.filename == "":
            flash("Файл не выбран", "error")
            return render_template(
                "import.html", project=project,
                unread_count=get_unread_count(current_user.id),
            )

        filename = file.filename.lower()
        try:
            if filename.endswith(".xlsx"):
                validate_xlsx_structure(file.stream)
                file.stream.seek(0)
                result = import_from_xlsx(file.stream, project_id, current_user.id)
            elif filename.endswith(".csv"):
                result = import_from_csv(file.stream, project_id, current_user.id)
            elif filename.endswith(".json"):
                result = import_from_json(file.stream, project_id, current_user.id)
            else:
                flash("Неподдерживаемый формат файла", "error")
                return render_template(
                    "import.html", project=project,
                    unread_count=get_unread_count(current_user.id),
                )

            msg = f"Импорт завершён. Требований: {result.get('imported_requirements', 0)}"
            if result.get("imported_links", 0) > 0:
                msg += f", связей: {result['imported_links']}"
            if result.get("errors"):
                msg += f". Ошибок: {len(result['errors'])}"
            flash(msg, "success")
            return redirect(
                url_for("requirements.list_requirements", project_id=project_id)
            )

        except ImportExportError as exc:
            flash(exc.message, "error")
        except Exception as exc:
            flash(f"Ошибка импорта: {exc}", "error")

    return render_template(
        "import.html", project=project,
        unread_count=get_unread_count(current_user.id),
    )


@ie_bp.route("/projects/<int:project_id>/export/<string:fmt>")
@login_required
def export_data(project_id, fmt):
    """
    Назначение: Экспорт данных в файл
    Id требования: Functional.ImportExport.Export
    """
    project = Project.query.get_or_404(project_id)

    if not current_user.can_import_export():
        flash("Недостаточно прав для экспорта", "error")
        return redirect(url_for("requirements.list_requirements", project_id=project_id))

    if fmt == "xlsx":
        output = export_to_xlsx(project_id, current_user.id)
        return send_file(
            output,
            mimetype="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
            as_attachment=True,
            download_name=f"{project.name}_export.xlsx",
        )
    elif fmt == "csv":
        content = export_to_csv(project_id, current_user.id)
        return Response(
            content,
            mimetype="text/csv",
            headers={
                "Content-Disposition": f"attachment; filename={project.name}_export.csv"
            },
        )
    elif fmt == "json":
        content = export_to_json(project_id, current_user.id)
        return Response(
            content,
            mimetype="application/json",
            headers={
                "Content-Disposition": f"attachment; filename={project.name}_export.json"
            },
        )

    flash("Неподдерживаемый формат экспорта", "error")
    return redirect(url_for("requirements.list_requirements", project_id=project_id))
