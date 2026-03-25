cmake_minimum_required(VERSION 3.16)
project(sut_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG        v0.18.3
)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(httplib json)

find_package(OpenSSL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED IMPORTED_TARGET libpqxx)

add_executable(sut_backend
    src/main.cpp
    src/database.cpp
    src/session_manager.cpp
    src/password_utils.cpp
    src/services/audit_service.cpp
    src/services/user_service.cpp
    src/services/requirement_service.cpp
    src/services/traceability_service.cpp
    src/services/change_service.cpp
    src/services/notification_service.cpp
    src/services/import_export_service.cpp
    src/api/auth_api.cpp
    src/api/projects_api.cpp
    src/api/requirements_api.cpp
    src/api/tracelinks_api.cpp
    src/api/changes_api.cpp
    src/api/notifications_api.cpp
    src/api/audit_api.cpp
    src/api/users_api.cpp
)

target_include_directories(sut_backend PRIVATE src)
target_link_libraries(sut_backend PRIVATE
    httplib::httplib
    nlohmann_json::nlohmann_json
    OpenSSL::SSL
    OpenSSL::Crypto
    PkgConfig::LIBPQXX
)
