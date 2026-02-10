#!/bin/bash

usage() {
    echo "Use: $0 <POSTGRESQL|MYSQL|MARIADB> <BINDING> [NODEJS|PHP|PYTHON]"
    echo "Use: $0 <POSTGRESQL|MYSQL|MARIADB> <CROWCPP> [ENDPOINT|MIDDLEWARE]"
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

if [ "$#" -lt 2 ]; then
    echo "Error: at least two arguments expected"
    usage
fi

DB_BACKEND="$1"

if ! validate_db_type "$DB_BACKEND"; then
    echo "Error: Invalid database type '$DB_BACKEND'"
    usage
fi

PROJECT_TYPE="$2"

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
    if [ "$#" -ne 3 ]; then
        usage
    else
      OPTION="$3"
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

cmake -G Ninja -DCAOS_DB_BACKEND="$DB_BACKEND" -DCAOS_PROJECT_TYPE="$PROJECT_TYPE" -D"$OPTION_VAR"="$OPTION" "$SOURCE_DIR"  #./../

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
