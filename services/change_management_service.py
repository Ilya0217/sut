"""
Модуль: change_management_service.py
Назначение: Управление процессом изменений требований
Автор: Разработчик
Дата создания: 20.03.2026
Требования: FR-11, Functional.ConfigurationManagement.BaselineAndChange,
             LLR_BaselineManager_CreateBaseline_01, LLR_BaselineManager_ProcessChange_01
"""

from datetime import datetime, timezone

from models import Baseline, ChangeRequest, Requirement, db
from services.audit_service import log_action
from services.notification_service import create_notification


class ChangeError(Exception):
    """Базовое исключение для ошибок управления изменениями."""

    def __init__(self, code, message):
        self.code = code
        self.message = message
        super().__init__(message)


def create_baseline(project_id, name, requirement_ids, user_id, description=""):
    """
    Назначение: Создание базовой версии набора требований
    Id требования: LLR_BaselineManager_CreateBaseline_01
    Входные данные: project_id, name, requirement_ids, user_id, description
    Выходные данные: объект Baseline
    """
    baseline = Baseline(
        name=name,
        project_id=project_id,
        description=description,
        created_by=user_id,
    )
    db.session.add(baseline)
    db.session.flush()

    for req_id in requirement_ids:
        req = Requirement.query.filter_by(
            id=req_id, project_id=project_id, is_deleted=False
        ).first()
        if req is not None:
            req.is_baseline = True
            req.baseline_id = baseline.id

    db.session.commit()

    log_action(
        user_id=user_id,
        action="create_baseline",
        object_type="baseline",
        object_id=baseline.id,
        new_value=f"{name} ({len(requirement_ids)} требований)",
    )
    return baseline


def create_change_request(requirement_id, user_id, justification,
                          changes_description, assigned_to=None):
    """
    Назначение: Создание запроса на изменение
    Id требования: LLR_BaselineManager_ProcessChange_01
    Входные данные: requirement_id, user_id, justification, changes_description
    Выходные данные: объект ChangeRequest
    """
    requirement = Requirement.query.filter_by(id=requirement_id).first()
    if requirement is None:
        raise ChangeError("ERR_REQUIREMENT_NOT_FOUND", "Требование не найдено")

    change_request = ChangeRequest(
        requirement_id=requirement_id,
        requested_by=user_id,
        assigned_to=assigned_to,
        justification=justification,
        changes_description=changes_description,
        status="pending",
    )
    db.session.add(change_request)
    db.session.commit()

    if assigned_to is not None:
        create_notification(
            user_id=assigned_to,
            event_type="CHANGE_REQUEST_ASSIGNED",
            message=(
                f"Вам назначен запрос на изменение #{change_request.id} "
                f"для требования {requirement.system_id}"
            ),
            related_object_type="change_request",
            related_object_id=change_request.id,
        )

    log_action(
        user_id=user_id,
        action="create_change_request",
        object_type="change_request",
        object_id=change_request.id,
        new_value=f"Для требования {requirement.system_id}",
    )
    return change_request


def approve_change_request(change_request_id, user_id, comment=""):
    """
    Назначение: Согласование запроса на изменение
    Id требования: Functional.ConfigurationManagement.BaselineAndChange
    Входные данные: change_request_id, user_id, comment
    Выходные данные: объект ChangeRequest
    """
    cr = ChangeRequest.query.filter_by(id=change_request_id).first()
    if cr is None:
        raise ChangeError("ERR_NOT_FOUND", "Запрос на изменение не найден")

    cr.status = "approved"
    cr.resolved_at = datetime.now(timezone.utc)
    cr.resolution_comment = comment
    db.session.commit()

    create_notification(
        user_id=cr.requested_by,
        event_type="CHANGE_REQUEST_APPROVED",
        message=f"Запрос на изменение #{cr.id} одобрен",
        related_object_type="change_request",
        related_object_id=cr.id,
    )

    log_action(
        user_id=user_id,
        action="approve_change_request",
        object_type="change_request",
        object_id=cr.id,
    )
    return cr


def reject_change_request(change_request_id, user_id, comment=""):
    """
    Назначение: Отклонение запроса на изменение
    Id требования: Functional.ConfigurationManagement.BaselineAndChange
    Входные данные: change_request_id, user_id, comment
    Выходные данные: объект ChangeRequest
    """
    cr = ChangeRequest.query.filter_by(id=change_request_id).first()
    if cr is None:
        raise ChangeError("ERR_NOT_FOUND", "Запрос на изменение не найден")

    cr.status = "rejected"
    cr.resolved_at = datetime.now(timezone.utc)
    cr.resolution_comment = comment
    db.session.commit()

    create_notification(
        user_id=cr.requested_by,
        event_type="CHANGE_REQUEST_REJECTED",
        message=f"Запрос на изменение #{cr.id} отклонён: {comment}",
        related_object_type="change_request",
        related_object_id=cr.id,
    )

    log_action(
        user_id=user_id,
        action="reject_change_request",
        object_type="change_request",
        object_id=cr.id,
    )
    return cr


def get_change_requests(project_id=None, status=None, page=1, per_page=50):
    """
    Назначение: Получение списка запросов на изменение
    Входные данные: project_id, status, page, per_page
    Выходные данные: пагинированный список
    """
    query = ChangeRequest.query
    if project_id is not None:
        query = query.join(Requirement).filter(
            Requirement.project_id == project_id
        )
    if status is not None:
        query = query.filter(ChangeRequest.status == status)
    query = query.order_by(ChangeRequest.created_at.desc())
    return query.paginate(page=page, per_page=per_page, error_out=False)
