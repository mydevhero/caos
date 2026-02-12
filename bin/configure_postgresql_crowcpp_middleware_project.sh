#!/bin/bash

PROJECT_NAME="${1:-my_app}"
DB_BACKEND=POSTGRESQL
PROJECT_TYPE=CROWCPP
CROWCPP_TYPE=MIDDLEWARE

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_SCRIPT="$SCRIPT_DIR/cmake_configure_project.sh"

if [ -x "$TARGET_SCRIPT" ]; then
    echo "Executing: $TARGET_SCRIPT with $DB_BACKEND $PROJECT_TYPE $CROWCPP_TYPE"
    "$TARGET_SCRIPT" "$PROJECT_NAME" "$DB_BACKEND" "$PROJECT_TYPE" "$CROWCPP_TYPE"
else
    echo "Error: $TARGET_SCRIPT not found or not executable."
    exit 1
fi

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
