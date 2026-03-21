"""
Модуль: traceability_service.py
Назначение: Управление связями трассировки между требованиями
Автор: Разработчик
Дата создания: 20.03.2026
Требования: FR-05, FR-06, FR-12, LLR_TraceLinkService_CreateLink_01-02,
             LLR_TraceLinkService_IntegrityCheck_01-03
"""

from models import Requirement, TraceLink, db
from services.audit_service import log_action
from services.notification_service import notify_integrity_failure


class TraceLinkError(Exception):
    """Базовое исключение для ошибок работы со связями."""

    def __init__(self, code, message):
        self.code = code
        self.message = message
        super().__init__(message)


def create_trace_link(source_id, target_id, link_type, user_id, description=""):
    """
    Назначение: Создание связи трассировки
    Id требования: LLR_TraceLinkService_CreateLink_01-02
    Входные данные: source_id, target_id, link_type, user_id, description
    Выходные данные: объект TraceLink или исключение
    """
    if source_id == target_id:
        raise TraceLinkError(
            "ERR_SELF_LINK",
            "Нельзя создать связь требования с самим собой",
        )

    source = Requirement.query.filter_by(id=source_id, is_deleted=False).first()
    if source is None:
        raise TraceLinkError(
            "ERR_REQUIREMENT_NOT_FOUND",
            f"Исходное требование #{source_id} не найдено",
        )

    target = Requirement.query.filter_by(id=target_id, is_deleted=False).first()
    if target is None:
        raise TraceLinkError(
            "ERR_REQUIREMENT_NOT_FOUND",
            f"Целевое требование #{target_id} не найдено",
        )

    if link_type not in TraceLink.VALID_TYPES:
        raise TraceLinkError(
            "ERR_INVALID_LINK_TYPE",
            f"Недопустимый тип связи: {link_type}",
        )

    existing = TraceLink.query.filter_by(
        source_req_id=source_id,
        target_req_id=target_id,
        link_type=link_type,
    ).first()
    if existing is not None:
        raise TraceLinkError(
            "ERR_DUPLICATE_LINK",
            "Связь с такими параметрами уже существует",
        )

    link = TraceLink(
        source_req_id=source_id,
        target_req_id=target_id,
        link_type=link_type,
        description=description,
        status="active",
        created_by=user_id,
    )
    db.session.add(link)
    db.session.commit()

    log_action(
        user_id=user_id,
        action="create",
        object_type="trace_link",
        object_id=link.id,
        new_value=f"{source.system_id} -> {target.system_id} ({link_type})",
    )
    return link


def delete_trace_link(link_id, user_id):
    """
    Назначение: Удаление связи трассировки
    Id требования: Functional.Traceability.LinkManagement.Manage
    Входные данные: link_id, user_id
    Выходные данные: True или исключение
    """
    link = TraceLink.query.filter_by(id=link_id).first()
    if link is None:
        raise TraceLinkError("ERR_LINK_NOT_FOUND", "Связь не найдена")

    log_action(
        user_id=user_id,
        action="delete",
        object_type="trace_link",
        object_id=link.id,
        old_value=f"{link.source_req_id} -> {link.target_req_id} ({link.link_type})",
    )

    db.session.delete(link)
    db.session.commit()
    return True


def get_links_for_requirement(requirement_id):
    """
    Назначение: Получение всех связей для требования
    Id требования: Interface.Software.TraceLinks
    Входные данные: requirement_id
    Выходные данные: список объектов TraceLink
    """
    return TraceLink.query.filter(
        (TraceLink.source_req_id == requirement_id)
        | (TraceLink.target_req_id == requirement_id)
    ).all()


def get_all_links_for_project(project_id):
    """
    Назначение: Получение всех связей проекта
    Входные данные: project_id
    Выходные данные: список TraceLink
    """
    return TraceLink.query.join(
        Requirement, TraceLink.source_req_id == Requirement.id
    ).filter(Requirement.project_id == project_id).all()


def check_integrity(requirement_id, user_id=None):
    """
    Назначение: Проверка целостности связей при изменении требования
    Id требования: LLR_TraceLinkService_IntegrityCheck_01-03
    Входные данные: requirement_id, user_id
    Выходные данные: список проблемных связей
    """
    links = TraceLink.query.filter(
        (TraceLink.source_req_id == requirement_id)
        | (TraceLink.target_req_id == requirement_id)
    ).all()

    issues = []
    for link in links:
        source = Requirement.query.filter_by(id=link.source_req_id).first()
        target = Requirement.query.filter_by(id=link.target_req_id).first()

        is_broken = False
        reason = ""

        if source is None or source.status == "deleted":
            is_broken = True
            reason = "Исходное требование удалено или не существует"
        if target is None or target.status == "deleted":
            is_broken = True
            reason = "Целевое требование удалено или не существует"

        if is_broken:
            link.status = "NEEDS_REVIEW"
            issues.append(link)

            responsible_id = None
            if source is not None and source.responsible_user_id is not None:
                responsible_id = source.responsible_user_id
            elif target is not None and target.responsible_user_id is not None:
                responsible_id = target.responsible_user_id

            if responsible_id is not None:
                notify_integrity_failure(
                    link_id=link.id,
                    requirement_id=requirement_id,
                    responsible_user_id=responsible_id,
                    reason=reason,
                )

    if issues:
        db.session.commit()

        if user_id is not None:
            log_action(
                user_id=user_id,
                action="integrity_check",
                object_type="trace_link",
                new_value=f"Найдено проблем: {len(issues)}",
            )

    return issues


def run_full_integrity_check(project_id, user_id):
    """
    Назначение: Полная проверка целостности связей проекта
    Id требования: Functional.Traceability.IntegrityCheck.OnDemand
    Входные данные: project_id, user_id
    Выходные данные: список проблемных связей
    """
    links = get_all_links_for_project(project_id)
    all_issues = []

    for link in links:
        source = Requirement.query.filter_by(id=link.source_req_id).first()
        target = Requirement.query.filter_by(id=link.target_req_id).first()

        is_broken = False
        reason = ""

        if source is None or source.is_deleted:
            is_broken = True
            reason = "Исходное требование удалено"
        elif target is None or target.is_deleted:
            is_broken = True
            reason = "Целевое требование удалено"

        if is_broken and link.status != "NEEDS_REVIEW":
            link.status = "NEEDS_REVIEW"
            all_issues.append({"link": link, "reason": reason})

            responsible_id = None
            if source is not None and source.responsible_user_id is not None:
                responsible_id = source.responsible_user_id
            elif target is not None and target.responsible_user_id is not None:
                responsible_id = target.responsible_user_id

            if responsible_id is not None:
                notify_integrity_failure(
                    link_id=link.id,
                    requirement_id=link.source_req_id,
                    responsible_user_id=responsible_id,
                    reason=reason,
                )

    if all_issues:
        db.session.commit()

    log_action(
        user_id=user_id,
        action="full_integrity_check",
        object_type="project",
        object_id=project_id,
        new_value=f"Проверено связей: {len(links)}, проблем: {len(all_issues)}",
    )

    return all_issues
