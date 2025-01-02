# -- GetVersionFromGit.cmake -------------------------------------------
#
# Этот скрипт:
# 1) Вызывает `git describe --tags --always`.
# 2) Извлекает (string(REGEX MATCH ...)) «X.Y.Z» до первого дефиса.
# 3) Устанавливает проектную версию в CMake-формате "X.Y.Z".
# 4) Сохраняет полную строку (например, "0.2.0-1-g959c4eb") в
#    переменной PROJECT_VERSION_FULL.
#
# Внимание: предполагается, что проект уже объявлен (или будет объявлен)
# и что данный скрипт вызывается ДО или ПОСЛЕ (зависит от вашего порядка)
# — см. пояснения ниже.

# Попробуем вытащить версию из GIT (git describe)
execute_process(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    RESULT_VARIABLE GIT_DESCRIBE_EXIT_CODE
)

# Если нет ошибок (== 0) — значит git describe отработал
if(GIT_DESCRIBE_EXIT_CODE EQUAL 0)
    set(_raw_version "${GIT_DESCRIBE}")
else()
    # Если репозиторий не содержит тегов или .git отсутствует,
    # тогда зададим версию по умолчанию
    set(_raw_version "0.0.0-no-git")
endif()

# Сохраняем полную строку в PROJECT_VERSION_FULL, чтобы потом можно было
# выводить/логировать/вписывать в бинарник
set(PROJECT_VERSION_FULL "${_raw_version}" CACHE INTERNAL "Full git version string")

# Извлекаем числовую часть вида X.Y.Z (до первого дефиса, если он есть).
# Если тег не содержит явных X.Y.Z — оставим "0.0.0"
string(REGEX MATCH "^[0-9]+\\.[0-9]+\\.[0-9]+" PARSED_VERSION "${_raw_version}")
if(NOT PARSED_VERSION)
    set(PARSED_VERSION "0.0.0")
endif()

# Сохраняем PARSED_VERSION в переменной PROJECT_VERSION_SEMVER (SemVer)
set(PROJECT_VERSION_SEMVER "${PARSED_VERSION}" CACHE INTERNAL "Semver version X.Y.Z")

# -------------------------------------------------------------------------
# Дальше: используйте PROJECT_VERSION_SEMVER и PROJECT_VERSION_FULL
#         в своем основном CMakeLists (или прямо тут).
#
# Например, можно прямо здесь объявить проект с нужной версией:
#
#   project(MyProject VERSION ${PROJECT_VERSION_SEMVER} LANGUAGES CXX)
#
# Но иногда это делают в главном CMakeLists, чтобы была гибкость.
# -------------------------------------------------------------------------

message(STATUS "Project semver parsed: ${PROJECT_VERSION_SEMVER}")
message(STATUS "Project full version : ${PROJECT_VERSION_FULL}")
