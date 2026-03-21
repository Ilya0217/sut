"""
Модуль: models.py
Назначение: Модели базы данных системы управления требованиями (СУТ)
Автор: Разработчик
Дата создания: 20.03.2026
Требования: Interface.Software.Database.PostgreSQL, LLR_RequirementService_Create_01-05,
             LLR_TraceLinkService_CreateLink_01-02, LLR_SecurityManager_Authenticate_01
"""

import uuid
from datetime import datetime, timezone

from flask_login import UserMixin
from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import check_password_hash, generate_password_hash

db = SQLAlchemy()


def generate_req_id():
    """
    Назначение: Генерация системного идентификатора требования
    Id требования: LLR_RequirementService_Create_01
    Входные данные: нет
    Выходные данные: строка формата REQ-{8 hex}
    """
    hex_part = uuid.uuid4().hex[:8].upper()
    return f"REQ-{hex_part}"


class User(UserMixin, db.Model):
    """
    Назначение: Модель пользователя системы
    Id требования: LLR_SecurityManager_Authenticate_01, Functional.Security.UserManagement
    """

    __tablename__ = "users"

    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False, index=True)
    email = db.Column(db.String(120), unique=True, nullable=False)
    password_hash = db.Column(db.String(256), nullable=False)
    role = db.Column(db.String(20), nullable=False, default="analyst")
    is_active_user = db.Column(db.Boolean, default=True)
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))

    ROLE_ANALYST = "analyst"
    ROLE_REVIEWER = "reviewer"
    ROLE_ADMIN = "admin"
    VALID_ROLES = [ROLE_ANALYST, ROLE_REVIEWER, ROLE_ADMIN]

    def set_password(self, password):
        """
        Назначение: Установка хэша пароля
        Id требования: Quality.Security.AccessAndStorage
        Входные данные: пароль (строка)
        Выходные данные: нет
        """
        self.password_hash = generate_password_hash(password)

    def check_password(self, password):
        """
        Назначение: Проверка пароля
        Id требования: LLR_SecurityManager_Authenticate_01
        Входные данные: пароль (строка)
        Выходные данные: True/False
        """
        return check_password_hash(self.password_hash, password)

    def can_create(self):
        return self.role in (self.ROLE_ANALYST, self.ROLE_ADMIN)

    def can_edit(self):
        return self.role in (self.ROLE_ANALYST, self.ROLE_ADMIN)

    def can_delete(self):
        return self.role in (self.ROLE_ANALYST, self.ROLE_ADMIN)

    def can_manage_links(self):
        return self.role in (self.ROLE_ANALYST, self.ROLE_ADMIN)

    def can_approve_changes(self):
        return self.role in (self.ROLE_REVIEWER, self.ROLE_ADMIN)

    def can_manage_users(self):
        return self.role == self.ROLE_ADMIN

    def can_import_export(self):
        return self.role in (self.ROLE_ANALYST, self.ROLE_ADMIN)

    def can_run_integrity_check(self):
        return self.role in (self.ROLE_ANALYST, self.ROLE_ADMIN)


class Project(db.Model):
    """
    Назначение: Модель проекта
    Id требования: Interface.Software.Projects
    """

    __tablename__ = "projects"

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(200), nullable=False)
    description = db.Column(db.Text, default="")
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    created_by = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)

    requirements = db.relationship("Requirement", backref="project", lazy="dynamic")
    custom_attributes = db.relationship(
        "ProjectCustomAttribute", backref="project", lazy="dynamic"
    )


class ProjectCustomAttribute(db.Model):
    """
    Назначение: Пользовательские атрибуты проекта
    Id требования: FR-01
    """

    __tablename__ = "project_custom_attributes"

    id = db.Column(db.Integer, primary_key=True)
    project_id = db.Column(db.Integer, db.ForeignKey("projects.id"), nullable=False)
    name = db.Column(db.String(100), nullable=False)
    attr_type = db.Column(db.String(50), default="text")


class Requirement(db.Model):
    """
    Назначение: Модель требования
    Id требования: Functional.RequirementManagement.Create,
                   LLR_RequirementService_Create_01-05
    """

    __tablename__ = "requirements"

    id = db.Column(db.Integer, primary_key=True)
    system_id = db.Column(
        db.String(20), unique=True, nullable=False, default=generate_req_id, index=True
    )
    custom_id = db.Column(db.String(50), nullable=True, index=True)
    project_id = db.Column(db.Integer, db.ForeignKey("projects.id"), nullable=False)
    title = db.Column(db.String(300), nullable=False)
    text = db.Column(db.Text, nullable=False)
    category = db.Column(db.String(100), nullable=False)
    priority = db.Column(db.String(50), nullable=False)
    status = db.Column(db.String(50), nullable=False, default="draft")
    parent_id = db.Column(db.Integer, db.ForeignKey("requirements.id"), nullable=True)
    responsible_user_id = db.Column(
        db.Integer, db.ForeignKey("users.id"), nullable=True
    )
    version = db.Column(db.Integer, default=1)
    is_baseline = db.Column(db.Boolean, default=False)
    baseline_id = db.Column(db.Integer, db.ForeignKey("baselines.id"), nullable=True)
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    updated_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    created_by = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)
    updated_by = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=True)
    is_deleted = db.Column(db.Boolean, default=False)
    deleted_at = db.Column(db.DateTime, nullable=True)

    VALID_STATUSES = [
        "draft", "active", "approved", "implemented",
        "verified", "deleted", "deprecated"
    ]
    VALID_PRIORITIES = ["critical", "high", "medium", "low"]
    VALID_CATEGORIES = [
        "functional", "non_functional", "interface",
        "performance", "security", "derived"
    ]

    children = db.relationship(
        "Requirement", backref=db.backref("parent", remote_side="Requirement.id"),
        lazy="dynamic"
    )
    responsible_user = db.relationship(
        "User", foreign_keys=[responsible_user_id], lazy="joined"
    )
    creator = db.relationship("User", foreign_keys=[created_by], lazy="joined")
    custom_values = db.relationship(
        "RequirementCustomValue", backref="requirement", lazy="dynamic",
        cascade="all, delete-orphan"
    )

    __table_args__ = (
        db.UniqueConstraint("custom_id", "project_id", name="uq_custom_id_project"),
    )


class RequirementCustomValue(db.Model):
    """
    Назначение: Значения пользовательских атрибутов требования
    Id требования: FR-01
    """

    __tablename__ = "requirement_custom_values"

    id = db.Column(db.Integer, primary_key=True)
    requirement_id = db.Column(
        db.Integer, db.ForeignKey("requirements.id"), nullable=False
    )
    attribute_id = db.Column(
        db.Integer, db.ForeignKey("project_custom_attributes.id"), nullable=False
    )
    value = db.Column(db.Text, default="")


class RequirementHistory(db.Model):
    """
    Назначение: История изменений требований
    Id требования: LLR_RequirementService_Edit_02,
                   Functional.RequirementManagement.EditAndHistory
    """

    __tablename__ = "requirement_history"

    id = db.Column(db.Integer, primary_key=True)
    requirement_id = db.Column(
        db.Integer, db.ForeignKey("requirements.id"), nullable=False
    )
    user_id = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)
    event_type = db.Column(db.String(50), nullable=False)
    attribute_name = db.Column(db.String(100), nullable=True)
    old_value = db.Column(db.Text, nullable=True)
    new_value = db.Column(db.Text, nullable=True)
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))

    user = db.relationship("User", lazy="joined")
    requirement = db.relationship("Requirement", backref="history_entries")


class TraceLink(db.Model):
    """
    Назначение: Связь трассировки между требованиями
    Id требования: Functional.Traceability.LinkManagement.Create,
                   LLR_TraceLinkService_CreateLink_01-02
    """

    __tablename__ = "trace_links"

    id = db.Column(db.Integer, primary_key=True)
    source_req_id = db.Column(
        db.Integer, db.ForeignKey("requirements.id"), nullable=False
    )
    target_req_id = db.Column(
        db.Integer, db.ForeignKey("requirements.id"), nullable=False
    )
    link_type = db.Column(db.String(50), nullable=False, default="derives_from")
    description = db.Column(db.Text, default="")
    status = db.Column(db.String(50), default="active")
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    created_by = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)

    VALID_TYPES = ["derives_from", "satisfies", "verifies", "implements"]
    VALID_STATUSES = ["active", "NEEDS_REVIEW", "deleted"]

    source_req = db.relationship(
        "Requirement", foreign_keys=[source_req_id], backref="outgoing_links"
    )
    target_req = db.relationship(
        "Requirement", foreign_keys=[target_req_id], backref="incoming_links"
    )
    creator = db.relationship("User", foreign_keys=[created_by])

    __table_args__ = (
        db.UniqueConstraint(
            "source_req_id", "target_req_id", "link_type",
            name="uq_trace_link"
        ),
    )


class ChangeRequest(db.Model):
    """
    Назначение: Запрос на изменение
    Id требования: Functional.ConfigurationManagement.BaselineAndChange,
                   LLR_BaselineManager_ProcessChange_01
    """

    __tablename__ = "change_requests"

    id = db.Column(db.Integer, primary_key=True)
    requirement_id = db.Column(
        db.Integer, db.ForeignKey("requirements.id"), nullable=False
    )
    requested_by = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)
    assigned_to = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=True)
    status = db.Column(db.String(50), default="pending")
    justification = db.Column(db.Text, nullable=False)
    changes_description = db.Column(db.Text, nullable=False)
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    resolved_at = db.Column(db.DateTime, nullable=True)
    resolution_comment = db.Column(db.Text, nullable=True)

    VALID_STATUSES = ["pending", "approved", "rejected", "in_progress", "completed"]

    requirement = db.relationship("Requirement", backref="change_requests")
    requester = db.relationship("User", foreign_keys=[requested_by])
    assignee = db.relationship("User", foreign_keys=[assigned_to])


class Baseline(db.Model):
    """
    Назначение: Базовая версия набора требований
    Id требования: LLR_BaselineManager_CreateBaseline_01
    """

    __tablename__ = "baselines"

    id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String(200), nullable=False)
    project_id = db.Column(db.Integer, db.ForeignKey("projects.id"), nullable=False)
    description = db.Column(db.Text, default="")
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    created_by = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)

    project = db.relationship("Project", backref="baselines")
    requirements = db.relationship("Requirement", backref="baseline", lazy="dynamic")


class Notification(db.Model):
    """
    Назначение: Уведомления пользователей
    Id требования: Functional.NotificationSystem.Alerts,
                   LLR_NotificationService_DeliverAlert_01
    """

    __tablename__ = "notifications"

    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)
    event_type = db.Column(db.String(100), nullable=False)
    message = db.Column(db.Text, nullable=False)
    is_read = db.Column(db.Boolean, default=False)
    related_object_type = db.Column(db.String(50), nullable=True)
    related_object_id = db.Column(db.Integer, nullable=True)
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))

    user = db.relationship("User", backref="notifications")


class AuditLog(db.Model):
    """
    Назначение: Журнал аудита
    Id требования: Functional.Audit.Logging, LLR_SecurityManager_Audit_01
    """

    __tablename__ = "audit_log"

    id = db.Column(db.Integer, primary_key=True)
    user_id = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=True)
    action = db.Column(db.String(100), nullable=False)
    object_type = db.Column(db.String(50), nullable=False)
    object_id = db.Column(db.String(50), nullable=True)
    old_value = db.Column(db.Text, nullable=True)
    new_value = db.Column(db.Text, nullable=True)
    context = db.Column(db.String(200), nullable=True)
    ip_address = db.Column(db.String(45), nullable=True)
    created_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))

    user = db.relationship("User", lazy="joined")


class ResponsibilityHistory(db.Model):
    """
    Назначение: История назначения ответственных лиц
    Id требования: Functional.ResponsibilityManagement.Assignment
    """

    __tablename__ = "responsibility_history"

    id = db.Column(db.Integer, primary_key=True)
    requirement_id = db.Column(
        db.Integer, db.ForeignKey("requirements.id"), nullable=False
    )
    previous_user_id = db.Column(
        db.Integer, db.ForeignKey("users.id"), nullable=True
    )
    new_user_id = db.Column(
        db.Integer, db.ForeignKey("users.id"), nullable=False
    )
    changed_by = db.Column(db.Integer, db.ForeignKey("users.id"), nullable=False)
    changed_at = db.Column(db.DateTime, default=lambda: datetime.now(timezone.utc))
    last_approved_at = db.Column(db.DateTime, nullable=True)
