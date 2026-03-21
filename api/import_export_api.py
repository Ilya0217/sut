"""
Модуль: import_export_api.py
Назначение: REST API импорта и экспорта данных
Автор: Разработчик
Дата создания: 21.03.2026
Требования: Interface.Software.ImportExport, FR-09, FR-10
"""

from flask import Blueprint, Response, jsonify, request, send_file
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

ie_api = Blueprint("import_export_api", __name__, url_prefix="/api/projects/<int:project_id>")


@ie_api.route("/import", methods=["POST"])
@login_required
def import_data(project_id):
    """
    Назначение: Импорт данных из файла
    Id требования: Functional.ImportExport.Import
    """
    if not current_user.can_import_export():
        return jsonify({"error": "Недостаточно прав"}), 403

    file = request.files.get("file")
    if file is None or file.filename == "":
        return jsonify({"error": "Файл не выбран"}), 400

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
            return jsonify({"error": "Неподдерживаемый формат"}), 400

        return jsonify(result)

    except ImportExportError as exc:
        return jsonify({"error": exc.message, "code": exc.code}), 400
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500


@ie_api.route("/export/<string:fmt>", methods=["GET"])
@login_required
def export_data(project_id, fmt):
    """
    Назначение: Экспорт данных
    Id требования: Functional.ImportExport.Export
    """
    if not current_user.can_import_export():
        return jsonify({"error": "Недостаточно прав"}), 403

    project = Project.query.get_or_404(project_id)

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
            headers={"Content-Disposition": f"attachment; filename={project.name}_export.csv"},
        )
    elif fmt == "json":
        content = export_to_json(project_id, current_user.id)
        return Response(
            content,
            mimetype="application/json",
            headers={"Content-Disposition": f"attachment; filename={project.name}_export.json"},
        )

    return jsonify({"error": "Неподдерживаемый формат"}), 400
