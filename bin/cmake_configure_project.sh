#!/bin/bash

usage() {
    echo "Use: $0 <PROJECT_NAME> <POSTGRESQL|MYSQL|MARIADB> <BINDING> [NODEJS|PHP|PYTHON]"
    echo "Use: $0 <PROJECT_NAME> <POSTGRESQL|MYSQL|MARIADB> <CROWCPP> [ENDPOINT|MIDDLEWARE]"
    exit 1
}

validate_db_type() {
    case "$1" in
        POSTGRESQL|MYSQL|MARIADB) return 0 ;;
        *) return 1 ;;
    esac
}

validate_binding_language() {
    case "$1" in
        NODEJS|PHP|PYTHON) return 0 ;;
        *) return 1 ;;
    esac
}

validate_crowcpp_type() {
    case "$1" in
        MIDDLEWARE|ENDPOINT) return 0 ;;
        *) return 1 ;;
    esac
}

mk_CMakeLists() {
if [ ! -f  "${PROJECT_DIR}/CMakeLists.txt" ]; then
cat << EOF > "${PROJECT_DIR}/CMakeLists.txt"
cmake_minimum_required(VERSION 3.5)

include(\${CMAKE_SOURCE_DIR}/CAOSDBA/cmake/policy.cmake)
include(\${CMAKE_SOURCE_DIR}/CAOSDBA/cmake/compiler.cmake)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "release")
endif()

project(${PROJECT_NAME} VERSION 0.0.1 LANGUAGES CXX)

include(\${CMAKE_SOURCE_DIR}/CAOSDBA/cmake/config.cmake)

include(\${CMAKE_SOURCE_DIR}/cmake/app.cmake)
EOF
fi
}

if [ "$#" -lt 3 ]; then
    echo "Error: at least three arguments expected"
    usage
fi

PROJECT_NAME="$1"
DB_BACKEND="$2"

if ! validate_db_type "$DB_BACKEND"; then
    echo "Error: Invalid database type '$DB_BACKEND'"
    usage
fi

PROJECT_TYPE="$3"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR/.."
PROJECT_DIR="$SOURCE_DIR/.."
RELEASE_DIR="$PROJECT_DIR/build/release"

mkdir -p "${RELEASE_DIR}"

if [ -d "${RELEASE_DIR}" ]; then
    echo "Created directory ${RELEASE_DIR}"
    cd "${RELEASE_DIR}"
else
    echo "Can't change directory to ${RELEASE_DIR}"
    exit 1;
fi

if [[ "$PROJECT_TYPE" == "BINDING" || "$PROJECT_TYPE" == "CROWCPP" ]]; then
    if [ "$#" -ne 4 ]; then
        usage
    else
      OPTION="$4"
    fi
fi

if [ "$PROJECT_TYPE" == "BINDING" ]; then
    if ! validate_binding_language "$OPTION"; then
        echo "Error: Invalid binding language '$OPTION'"
        usage
    fi

    OPTION_VAR=CAOS_BINDING_LANGUAGE

elif [ "$PROJECT_TYPE" == "CROWCPP" ]; then
    if ! validate_crowcpp_type "$OPTION"; then
        echo "Error: Invalid crowcpp type '$OPTION'"
        usage
    fi

    OPTION_VAR=CAOS_CROWCPP_TYPE

else
    usage
fi

mk_CMakeLists

cmake -G Ninja -DCAOS_DB_BACKEND="$DB_BACKEND" -DCAOS_PROJECT_TYPE="$PROJECT_TYPE" -D"$OPTION_VAR"="$OPTION" "$PROJECT_DIR"

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
