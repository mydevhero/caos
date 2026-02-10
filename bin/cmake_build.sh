#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR/.."
PROJECT_DIR="$SOURCE_DIR/.."
RELEASE_DIR="$PROJECT_DIR/build/release"

cmake --build "$RELEASE_DIR"

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
