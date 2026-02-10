#!/bin/bash

DB_BACKEND=POSTGRESQL
PROJECT_TYPE=BINDING
BINDING_LANGUAGE=NODEJS

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_SCRIPT="$SCRIPT_DIR/cmake_configure_project.sh"

if [ -x "$TARGET_SCRIPT" ]; then
    echo "Executing: $TARGET_SCRIPT with $DB_BACKEND $PROJECT_TYPE $BINDING_LANGUAGE"
    "$TARGET_SCRIPT" "$DB_BACKEND" "$PROJECT_TYPE" "$BINDING_LANGUAGE"
else
    echo "Error: $TARGET_SCRIPT not found or not executable."
    exit 1
fi

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
