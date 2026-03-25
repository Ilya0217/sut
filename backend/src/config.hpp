/*
Модуль: config.hpp
Назначение: Конфигурация приложения СУТ
Автор: Разработчик
Дата создания: 21.03.2026
*/

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdlib>
#include <string>

struct Config {
    std::string database_url;
    std::string secret_key;
    std::string cors_origin;
    int port = 5000;

    /*
    Назначение: Загрузка конфигурации из переменных окружения
    Id требования: Архитектура ПО (ApiGateway)
    Входные данные: нет
    Выходные данные: заполненная структура Config
    */
    static Config load()
    {
        Config cfg;
        cfg.database_url = get_env(
            "DATABASE_URL",
            "postgresql://sut_user:sut_password@localhost:5432/sut"
        );
        cfg.secret_key = get_env(
            "SECRET_KEY",
            "sut-secret-key-change-in-production"
        );
        cfg.cors_origin = get_env("CORS_ORIGIN", "http://localhost:3000");
        cfg.port = std::stoi(get_env("PORT", "5000"));
        return cfg;
    }

private:
    static std::string get_env(const char* name, const char* fallback)
    {
        const char* val = std::getenv(name);
        return (val != nullptr) ? std::string(val) : std::string(fallback);
    }
};

#endif
