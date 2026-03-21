"""
Модуль: requirement_service.py
Назначение: Управление полным жизненным циклом требований
Автор: Разработчик
Дата создания: 20.03.2026
Требования: FR-01-FR-04, FR-07, FR-08, LLR_RequirementService_Create_01-05,
             LLR_RequirementService_Edit_01-02, LLR_RequirementService_Delete_01-02,
             LLR_RequirementService_View_01
"""

from datetime import datetime, timezone

from sqlalchemy import or_

from models import (
    Requirement, RequirementHistory, ResponsibilityHistory,
    db, generate_req_id,
)
from services.audit_service import log_action
from services.notification_service import create_notification


class RequirementError(Exception):
    """Базовое исключение для ошибок работы с требованиями."""

    def __init__(self, code, message, field=None):
        self.code = code
        self.message = message
        self.field = field
        super().__init__(message)


def create_requirement(project_id, user_data, user_id):
    """
    Назначение: Создание нового требования с валидацией
    Id требования: LLR_RequirementService_Create_01-05
    Входные данные: project_id, user_data (dict), user_id
    Выходные данные: объект Requirement или исключение
    """
    required_fields = ["title", "text", "category", "priority", "status"]
    for field in required_fields:
        value = user_data.get(field, "")
        if not value or not str(value).strip():
            raise RequirementError(
                "ERR_MISSING_REQUIRED_FIELD",
                f"Обязательное поле '{field}' отсутствует или пусто",
                field=field,
            )

    category = user_data.get("category", "")
    if category not in Requirement.VALID_CATEGORIES:
        raise RequirementError(
            "ERR_INVALID_CATEGORY",
            f"Недопустимая категория: {category}",
            field="category",
        )

    priority = user_data.get("priority", "")
    if priority not in Requirement.VALID_PRIORITIES:
        raise RequirementError(
            "ERR_INVALID_PRIORITY",
            f"Недопустимый приоритет: {priority}",
            field="priority",
        )

    status = user_data.get("status", "")
    if status not in Requirement.VALID_STATUSES:
        raise RequirementError(
            "ERR_INVALID_STATUS",
            f"Недопустимый статус: {status}",
            field="status",
        )

    custom_id = user_data.get("custom_id", "").strip()
    if custom_id:
        existing = Requirement.query.filter_by(
            custom_id=custom_id, project_id=project_id
        ).first()
        if existing is not None:
            raise RequirementError(
                "ERR_DUPLICATE_CUSTOM_ID",
                f"Пользовательский ID '{custom_id}' уже используется в проекте",
            )

    now = datetime.now(timezone.utc)
    requirement = Requirement(
        system_id=generate_req_id(),
        custom_id=custom_id if custom_id else None,
        project_id=project_id,
        title=user_data["title"].strip(),
        text=user_data["text"].strip(),
        category=user_data["category"],
        priority=user_data["priority"],
        status=user_data["status"],
        parent_id=user_data.get("parent_id"),
        responsible_user_id=user_data.get("responsible_user_id"),
        created_at=now,
        updated_at=now,
        created_by=user_id,
        updated_by=user_id,
        version=1,
    )

    history_entry = RequirementHistory(
        requirement=requirement,
        user_id=user_id,
        event_type="CREATED",
        attribute_name=None,
        old_value=None,
        new_value=f"Требование создано: {requirement.title}",
        created_at=now,
    )

    db.session.add(requirement)
    db.session.add(history_entry)
    db.session.commit()

    log_action(
        user_id=user_id,
        action="create",
        object_type="requirement",
        object_id=requirement.system_id,
        new_value=requirement.title,
    )
    return requirement


def edit_requirement(requirement_id, user_data, user_id):
    """
    Назначение: Редактирование существующего требования
    Id требования: LLR_RequirementService_Edit_01-02
    Входные данные: requirement_id, user_data (dict), user_id
    Выходные данные: объект Requirement или исключение
    """
    requirement = Requirement.query.filter_by(
        id=requirement_id, is_deleted=False
    ).with_for_update().first()

    if requirement is None:
        raise RequirementError(
            "ERR_REQUIREMENT_NOT_FOUND",
            "Требование не найдено",
        )

    if requirement.is_baseline:
        pending_cr = requirement.change_requests.filter_by(status="approved").first()
        if pending_cr is None:
            raise RequirementError(
                "ERR_BASELINE_LOCKED",
                "Требование находится в базовой версии. Создайте запрос на изменение.",
            )

    now = datetime.now(timezone.utc)
    editable_fields = ["title", "text", "category", "priority", "status",
                       "custom_id", "parent_id", "responsible_user_id"]
    changed_fields = []

    for field in editable_fields:
        if field not in user_data:
            continue
        old_value = getattr(requirement, field)
        new_value = user_data[field]
        if str(old_value) != str(new_value):
            history_entry = RequirementHistory(
                requirement_id=requirement.id,
                user_id=user_id,
                event_type="MODIFIED",
                attribute_name=field,
                old_value=str(old_value) if old_value is not None else "",
                new_value=str(new_value) if new_value is not None else "",
                created_at=now,
            )
            db.session.add(history_entry)
            setattr(requirement, field, new_value)
            changed_fields.append(field)

    if "responsible_user_id" in user_data:
        old_resp = requirement.responsible_user_id
        new_resp = user_data["responsible_user_id"]
        if old_resp != new_resp:
            resp_history = ResponsibilityHistory(
                requirement_id=requirement.id,
                previous_user_id=old_resp,
                new_user_id=new_resp,
                changed_by=user_id,
                changed_at=now,
            )
            db.session.add(resp_history)
            if new_resp is not None:
                create_notification(
                    user_id=new_resp,
                    event_type="RESPONSIBILITY_ASSIGNED",
                    message=(
                        f"Вы назначены ответственным за требование "
                        f"{requirement.system_id}"
                    ),
                    related_object_type="requirement",
                    related_object_id=requirement.id,
                )

    if changed_fields:
        requirement.updated_at = now
        requirement.updated_by = user_id
        requirement.version += 1
        db.session.commit()

        log_action(
            user_id=user_id,
            action="edit",
            object_type="requirement",
            object_id=requirement.system_id,
            new_value=", ".join(changed_fields),
        )

    return requirement


def soft_delete_requirement(requirement_id, user_id):
    """
    Назначение: Мягкое удаление требования (перевод в статус deleted)
    Id требования: LLR_RequirementService_Delete_01
    Входные данные: requirement_id, user_id
    Выходные данные: объект Requirement или исключение
    """
    requirement = Requirement.query.filter_by(
        id=requirement_id, is_deleted=False
    ).first()
    if requirement is None:
        raise RequirementError(
            "ERR_REQUIREMENT_NOT_FOUND", "Требование не найдено"
        )

    now = datetime.now(timezone.utc)
    requirement.is_deleted = True
    requirement.deleted_at = now
    requirement.status = "deleted"
    requirement.updated_at = now
    requirement.updated_by = user_id

    history_entry = RequirementHistory(
        requirement_id=requirement.id,
        user_id=user_id,
        event_type="DELETED",
        attribute_name="status",
        old_value="active",
        new_value="deleted",
        created_at=now,
    )
    db.session.add(history_entry)
    db.session.commit()

    log_action(
        user_id=user_id,
        action="soft_delete",
        object_type="requirement",
        object_id=requirement.system_id,
    )
    return requirement


def hard_delete_requirement(requirement_id, user_id):
    """
    Назначение: Безвозвратное удаление требования
    Id требования: LLR_RequirementService_Delete_02
    Входные данные: requirement_id, user_id
    Выходные данные: True или исключение
    """
    requirement = Requirement.query.filter_by(id=requirement_id).first()
    if requirement is None:
        raise RequirementError(
            "ERR_REQUIREMENT_NOT_FOUND", "Требование не найдено"
        )

    from models import User
    acting_user = db.session.get(User, user_id)
    is_responsible = (requirement.responsible_user_id == user_id)
    is_admin = (acting_user is not None and acting_user.role == User.ROLE_ADMIN)
    if not is_responsible and not is_admin:
        raise RequirementError(
            "ERR_FORBIDDEN",
            "Безвозвратное удаление разрешено только ответственному лицу или администратору",
        )

    log_action(
        user_id=user_id,
        action="hard_delete",
        object_type="requirement",
        object_id=requirement.system_id,
    )

    RequirementHistory.query.filter_by(requirement_id=requirement.id).delete()
    db.session.delete(requirement)
    db.session.commit()
    return True


def restore_requirement(requirement_id, user_id):
    """
    Назначение: Восстановление удалённого требования
    Id требования: Functional.RequirementManagement.Delete
    Входные данные: requirement_id, user_id
    Выходные данные: объект Requirement или исключение
    """
    requirement = Requirement.query.filter_by(
        id=requirement_id, is_deleted=True
    ).first()
    if requirement is None:
        raise RequirementError(
            "ERR_REQUIREMENT_NOT_FOUND", "Удалённое требование не найдено"
        )

    requirement.is_deleted = False
    requirement.deleted_at = None
    requirement.status = "draft"
    requirement.updated_at = datetime.now(timezone.utc)
    requirement.updated_by = user_id
    db.session.commit()

    log_action(
        user_id=user_id,
        action="restore",
        object_type="requirement",
        object_id=requirement.system_id,
    )
    return requirement


def get_requirements_list(project_id, filters=None, sort_by="created_at",
                          sort_order="desc", page=1, per_page=50,
                          include_deleted=False):
    """
    Назначение: Получение списка требований с фильтрацией и пагинацией
    Id требования: LLR_RequirementService_View_01, Functional.SearchAndFilter.Filtering
    Входные данные: project_id, filters, sort_by, sort_order, page, per_page
    Выходные данные: пагинированный список требований
    """
    query = Requirement.query.filter_by(project_id=project_id)

    if not include_deleted:
        query = query.filter_by(is_deleted=False)

    if filters is not None:
        if filters.get("status"):
            query = query.filter(Requirement.status == filters["status"])
        if filters.get("priority"):
            query = query.filter(Requirement.priority == filters["priority"])
        if filters.get("category"):
            query = query.filter(Requirement.category == filters["category"])
        if filters.get("responsible_user_id"):
            query = query.filter(
                Requirement.responsible_user_id == filters["responsible_user_id"]
            )

    sort_column = getattr(Requirement, sort_by, Requirement.created_at)
    if sort_order == "asc":
        query = query.order_by(sort_column.asc())
    else:
        query = query.order_by(sort_column.desc())

    return query.paginate(page=page, per_page=per_page, error_out=False)


def search_requirements(project_id, search_text, page=1, per_page=50):
    """
    Назначение: Текстовый поиск по требованиям
    Id требования: Functional.SearchAndFilter.Search
    Входные данные: project_id, search_text, page, per_page
    Выходные данные: пагинированный список результатов
    """
    pattern = f"%{search_text}%"
    query = Requirement.query.filter(
        Requirement.project_id == project_id,
        Requirement.is_deleted == False,
        or_(
            Requirement.system_id.ilike(pattern),
            Requirement.custom_id.ilike(pattern),
            Requirement.title.ilike(pattern),
            Requirement.text.ilike(pattern),
        ),
    )
    query = query.order_by(Requirement.updated_at.desc())
    return query.paginate(page=page, per_page=per_page, error_out=False)


def get_requirement_by_id(requirement_id):
    """
    Назначение: Получение требования по ID
    Входные данные: requirement_id
    Выходные данные: объект Requirement или None
    """
    return Requirement.query.filter_by(id=requirement_id).first()


def get_requirement_history(requirement_id, page=1, per_page=50):
    """
    Назначение: Получение истории изменений требования
    Id требования: Functional.RequirementManagement.EditAndHistory
    Входные данные: requirement_id, page, per_page
    Выходные данные: пагинированный список записей истории
    """
    return RequirementHistory.query.filter_by(
        requirement_id=requirement_id
    ).order_by(
        RequirementHistory.created_at.desc()
    ).paginate(page=page, per_page=per_page, error_out=False)
