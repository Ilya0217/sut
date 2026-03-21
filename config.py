"""
Модуль: config.py
Назначение: Конфигурация приложения СУТ
Автор: Разработчик
Дата создания: 20.03.2026
"""

import os


class Config:
    """Базовая конфигурация приложения."""

    SECRET_KEY = os.environ.get("SECRET_KEY", "sut-secret-key-change-in-production")
    SQLALCHEMY_DATABASE_URI = os.environ.get(
        "DATABASE_URL",
        "sqlite:///sut.db"
    )
    SQLALCHEMY_TRACK_MODIFICATIONS = False
    MAX_CONTENT_LENGTH = 16 * 1024 * 1024
    ITEMS_PER_PAGE = 50
    MAIL_SERVER = os.environ.get("MAIL_SERVER", "localhost")
    MAIL_PORT = int(os.environ.get("MAIL_PORT", 25))
