#!/bin/bash

# Function to display script usage
usage() {
    echo "Usage: $0 --init"
    echo "       $0 --purge"
    echo "       $1 PROJECT_NAME"
    echo ""
    echo "Options:"
    echo "  --init         Initialize project from CMake configuration files"
    echo "  --purge        Remove all generated project files"
    echo "  PROJECT_NAME   CMAKE PROJECT_NAME var"
    echo ""
    echo "Configuration is read from:"
    echo "  - cmake/db_backend.cmake (DB_BACKEND)"
    echo "  - cmake/project_type.cmake (PROJECT_TYPE)"
    echo "  - cmake/binding_language.cmake (BINDING_LANGUAGE, if PROJECT_TYPE is BINDING)"
    echo "  - cmake/crowcpp_type.cmake (CROWCPP_TYPE, if PROJECT_TYPE is CROWCPP)"
    exit 1
}

# Function to purge project files
purge_project() {
    echo "WARNING: This will delete all project files created by this script."
    echo "This includes:"
    echo "  - $PROJECT_DIR/build (entire directory)"
    echo "  - $PROJECT_DIR/dist (entire directory)"
    echo "  - $PROJECT_DIR/src (entire directory)"
    echo "  - $PROJECT_DIR/scripts (entire directory)"
    echo "  - $PROJECT_DIR/typescript (entire directory)"
    echo "  - $PROJECT_DIR/queries (entire directory)"
    echo "  - $PROJECT_DIR/schemas (entire directory)"
#    echo "  - Files in $PROJECT_DIR/cmake/"
    echo "  - $PROJECT_DIR/cmake (entire directory)"
    echo ""
    read -p "Are you sure you want to continue? (y/N): " confirm

    case $confirm in
        [yY])
            echo "Purging project files..."

            # Remove build/release directory
            if [ -d "$PROJECT_DIR/build" ]; then
                rm -rf "$PROJECT_DIR/build"
                echo "Removed $PROJECT_DIR/build"
            else
                echo "$PROJECT_DIR/build directory not found, skipping"
            fi

            # Remove src directory
            if [ -d "$PROJECT_DIR/src" ]; then
                rm -rf "$PROJECT_DIR/src"
                echo "Removed $PROJECT_DIR/src"
            else
                echo "$PROJECT_DIR/src directory not found, skipping"
            fi

            # Remove scripts directory
            if [ -d "$PROJECT_DIR/scripts" ]; then
                rm -rf "$PROJECT_DIR/scripts"
                echo "Removed $PROJECT_DIR/scripts"
            else
                echo "$PROJECT_DIR/scripts directory not found, skipping"
            fi

            # Remove typescript directory (new for Node.js TypeScript support)
            if [ -d "$PROJECT_DIR/typescript" ]; then
                rm -rf "$PROJECT_DIR/typescript"
                echo "Removed $PROJECT_DIR/typescript"
            else
                echo "$PROJECT_DIR/typescript directory not found, skipping"
            fi

            # Remove queries directory (for custom query definitions)
            if [ -d "$PROJECT_DIR/queries" ]; then
                rm -rf "$PROJECT_DIR/queries"
                echo "Removed $PROJECT_DIR/queries"
            else
                echo "$PROJECT_DIR/queries directory not found, skipping"
            fi

            # Remove schemas directory (for JSON schema validation)
            if [ -d "$PROJECT_DIR/schemas" ]; then
                rm -rf "$PROJECT_DIR/schemas"
                echo "Removed $PROJECT_DIR/schemas"
            else
                echo "$PROJECT_DIR/schemas directory not found, skipping"
            fi

            # Remove cmake files created by this script
            if [ -d "$PROJECT_DIR/cmake" ]; then
                rm -rf "$PROJECT_DIR/cmake"
                echo "Removed $PROJECT_DIR/cmake"
            else
                echo "$PROJECT_DIR/cmake directory not found, skipping"
            fi

            # Remove dist directory created by previous builds
            if [ -d "$PROJECT_DIR/dist" ]; then
                rm -rf "$PROJECT_DIR/dist"
                echo "Removed $PROJECT_DIR/dist"
            else
                echo "$PROJECT_DIR/dist directory not found, skipping"
            fi
#            cmake_files=("type_code.cmake" "app.cmake" "app_debug.cmake" "project_type.cmake"
#                         "binding_language.cmake" "db_backend.cmake" "crowcpp_type.cmake")
#            for file in "${cmake_files[@]}"; do
#                if [ -f "$PROJECT_DIR/cmake/$file" ]; then
#                    rm "$PROJECT_DIR/cmake/$file"
#                    echo "Removed $SOURCE_DIR/cmake/$file"
#                fi
#            done

            echo "Purge completed successfully"
            return 0
            ;;
        *)
            echo "Purge cancelled"
            exit 0
            ;;
    esac
}

# Function to read CMake variable from file
read_cmake_var() {
    local file=$1
    local var_name=$2

    if [ ! -f "$file" ]; then
        return 1
    fi

    # Extract value from set(VAR_NAME VALUE)
    local value=$(grep "^set($var_name" "$file" | sed -n "s/^set($var_name \(.*\)).*$/\1/p" | tr -d ' ')

    if [ -z "$value" ]; then
        return 1
    fi

    echo "$value"
    return 0
}

# Function to initialize project
init_project() {
    # Check if project is already initialized
    if [ -d "$PROJECT_DIR/src" ] || [ -f "$PROJECT_DIR/cmake/app.cmake" ]; then
        echo "Project already initialized, skipping setup"
        exit 0
    fi

    # Read database backend configuration
    DB_BACKEND=$(read_cmake_var "$PROJECT_DIR/cmake/db_backend.cmake" "DB_BACKEND")
    if [ $? -ne 0 ] || [ -z "$DB_BACKEND" ]; then
        echo "Error: Could not read DB_BACKEND from $PROJECT_DIR/cmake/db_backend.cmake"
        echo "Make sure the file exists and contains: set(DB_BACKEND <value>)"
        exit 1
    fi

    # Read project type configuration
    PROJECT_TYPE=$(read_cmake_var "$PROJECT_DIR/cmake/project_type.cmake" "PROJECT_TYPE")
    if [ $? -ne 0 ] || [ -z "$PROJECT_TYPE" ]; then
        echo "Error: Could not read PROJECT_TYPE from $PROJECT_DIR/cmake/project_type.cmake"
        echo "Make sure the file exists and contains: set(PROJECT_TYPE <value>)"
        exit 1
    fi

    # Validate database type and set proper directory name
    case $DB_BACKEND in
        POSTGRESQL)
            DB_DIR="PostgreSQL"
            ;;
        MYSQL)
            DB_DIR="MySQL"
            ;;
        MARIADB)
            DB_DIR="MariaDB"
            ;;
        *)
            echo "Error: Invalid database type '$DB_BACKEND' in db_backend.cmake"
            echo "Valid options: POSTGRESQL, MYSQL, MARIADB"
            exit 1
            ;;
    esac

    # Validate project type
    case $PROJECT_TYPE in
        STANDALONE)
            TEMPLATE_SUBDIR="$TEMPLATE_DIR/standalone"
            ;;
        BINDING)
            # Read binding language for BINDING type
            BINDING_LANGUAGE=$(read_cmake_var "$PROJECT_DIR/cmake/binding_language.cmake" "BINDING_LANGUAGE")
            if [ $? -ne 0 ] || [ -z "$BINDING_LANGUAGE" ]; then
                echo "Error: Could not read BINDING_LANGUAGE from $PROJECT_DIR/cmake/binding_language.cmake"
                echo "BINDING_LANGUAGE is required when PROJECT_TYPE is BINDING"
                exit 1
            fi

            # Validate and convert binding language to lowercase for directory
            case $BINDING_LANGUAGE in
                PHP)
                    LANG_DIR="php"
                    ;;
                NODEJS)
                    LANG_DIR="nodejs"
                    ;;
                PYTHON)
                    LANG_DIR="python"
                    ;;
                *)
                    echo "Error: Invalid binding language '$BINDING_LANGUAGE' in binding_language.cmake"
                    echo "Valid options: PHP, NODEJS, PYTHON"
                    exit 1
                    ;;
            esac

            TEMPLATE_SUBDIR="$TEMPLATE_DIR/bindings/$LANG_DIR"
            ;;
        CROWCPP)
            # Read CROWCPP type for CROWCPP projects
            CROWCPP_TYPE=$(read_cmake_var "$PROJECT_DIR/cmake/crowcpp_type.cmake" "CROWCPP_TYPE")
            if [ $? -ne 0 ] || [ -z "$CROWCPP_TYPE" ]; then
                echo "Error: Could not read CROWCPP_TYPE from $PROJECT_DIR/cmake/crowcpp_type.cmake"
                echo "CROWCPP_TYPE is required when PROJECT_TYPE is CROWCPP"
                exit 1
            fi

            # Validate CROWCPP type
            case $CROWCPP_TYPE in
                ENDPOINT)
                    CROWCPP_DIR="endpoint"
                    ;;
                MIDDLEWARE)
                    CROWCPP_DIR="middleware"
                    ;;
                *)
                    echo "Error: Invalid CROWCPP type '$CROWCPP_TYPE' in crowcpp_type.cmake"
                    echo "Valid options: ENDPOINT, MIDDLEWARE"
                    exit 1
                    ;;
            esac

            TEMPLATE_SUBDIR="$TEMPLATE_DIR/crowcpp/$CROWCPP_DIR"
            ;;
        *)
            echo "Error: Invalid project type '$PROJECT_TYPE' in project_type.cmake"
            echo "Valid options: STANDALONE, BINDING, CROWCPP"
            exit 1
            ;;
    esac

    # Check if template subdirectory exists
    if [ ! -d "$TEMPLATE_SUBDIR" ]; then
        echo "Error: Template directory $TEMPLATE_SUBDIR does not exist"
        exit 1
    fi

    echo "Initializing project with configuration:"
    echo "  Database: $DB_BACKEND ($DB_DIR)"
    echo "  Project type: $PROJECT_TYPE"
    if [ "$PROJECT_TYPE" = "BINDING" ]; then
        echo "  Language: $BINDING_LANGUAGE"
        if [ "$BINDING_LANGUAGE" = "NODEJS" ]; then
            echo "  TypeScript support: Included"
        fi
    elif [ "$PROJECT_TYPE" = "CROWCPP" ]; then
        echo "  CrowCpp type: $CROWCPP_TYPE"
    fi
    echo ""

    # Create cmake directory if not exists
    CMAKE_DIR="$PROJECT_DIR/cmake"
    mkdir -p "$CMAKE_DIR"

    # Copy template files based on project type
    case $PROJECT_TYPE in
        STANDALONE)
            # Copy type_code.cmake
            cp "$TEMPLATE_SUBDIR/cmake/type_code.cmake" "$CMAKE_DIR/"
            echo "Copied type_code.cmake"

            # Copy app.cmake
            cp "$TEMPLATE_SUBDIR/cmake/app.cmake" "$CMAKE_DIR/"
            echo "Copied app.cmake"

            # Copy app_debug.cmake
            cp "$TEMPLATE_SUBDIR/cmake/app_debug.cmake" "$CMAKE_DIR/"
            echo "Copied app_debug.cmake"

            # Copy Redis Cache Query.hpp
            REDIS_SRC="$TEMPLATE_SUBDIR/src/include/Cache/Redis/Query.hpp"
            REDIS_DEST="$PROJECT_DIR/src/include/Cache/Redis/Query.hpp"
            mkdir -p "$(dirname "$REDIS_DEST")"
            cp "$REDIS_SRC" "$REDIS_DEST"
            echo "Copied Redis Query.hpp"

            # Copy Database Query.hpp based on selected database
            DB_SRC="$TEMPLATE_SUBDIR/src/include/Database/$DB_DIR/Query.hpp"
            DB_DEST="$PROJECT_DIR/src/include/Database/$DB_DIR/Query.hpp"
            mkdir -p "$(dirname "$DB_DEST")"
            cp "$DB_SRC" "$DB_DEST"
            echo "Copied $DB_DIR Database Query.hpp"

            # Copy main.cpp
            mkdir -p "$PROJECT_DIR/src"
            cp "$TEMPLATE_SUBDir/src/main.cpp" "$PROJECT_DIR/src/"
            echo "Copied src/main.cpp"
            ;;

        BINDING)
            echo "Copying binding template structure..."

            # Copy everything except src/include directory
            for item in "$TEMPLATE_SUBDIR"/*; do
                item_name=$(basename "$item")
                if [ "$item_name" != "src" ]; then
                    cp -r "$item" "$PROJECT_DIR/"
                    echo "Copied $item_name"
                fi
            done

            # Handle src directory separately
            if [ -d "$TEMPLATE_SUBDIR/src" ]; then
                # Copy everything in src except include
                for item in "$TEMPLATE_SUBDIR/src"/*; do
                    item_name=$(basename "$item")
                    if [ "$item_name" != "include" ]; then
                        mkdir -p "$PROJECT_DIR/src"
                        cp -r "$item" "$PROJECT_DIR/src/"
                        echo "Copied src/$item_name"
                    fi
                done

                # Handle src/include directory
                if [ -d "$TEMPLATE_SUBDIR/src/include" ]; then
                    INCLUDE_SRC="$TEMPLATE_SUBDIR/src/include"
                    INCLUDE_DEST="$PROJECT_DIR/src/include"
                    mkdir -p "$INCLUDE_DEST"

                    # Copy files directly in include/ (like extension_config.h.in)
                    find "$INCLUDE_SRC" -maxdepth 1 -type f -exec cp {} "$INCLUDE_DEST/" \;

                    # Copy Cache directory
                    if [ -d "$INCLUDE_SRC/Cache" ]; then
                        cp -r "$INCLUDE_SRC/Cache" "$INCLUDE_DEST/"
                        echo "Copied src/include/Cache"
                    fi

                    # Copy only the selected database directory
                    if [ -d "$INCLUDE_SRC/Database/$DB_DIR" ]; then
                        mkdir -p "$INCLUDE_DEST/Database"
                        cp -r "$INCLUDE_SRC/Database/$DB_DIR" "$INCLUDE_DEST/Database/"
                        echo "Copied src/include/Database/$DB_DIR"
                    else
                        echo "Warning: Database template for $DB_DIR not found"
                    fi
                fi
            fi

            # Special handling for Node.js TypeScript support
            if [ "$BINDING_LANGUAGE" = "NODEJS" ] && [ -d "$TEMPLATE_SUBDIR/typescript" ]; then
                echo "Copying TypeScript template files..."
                cp -r "$TEMPLATE_SUBDIR/typescript" "$PROJECT_DIR/"
                echo "Copied TypeScript template files"
            fi

            # Check if queries/custom directory were copied
            if [ -d "$PROJECT_DIR/queries/custom" ]; then
                echo "Custom query definitions directory created: $PROJECT_DIR/queries/custom"
                echo "  Note: Add custom queries to $PROJECT_DIR/queries/custom/queries.yaml"
            fi

            # Check if schemas/custom directory were copied
            if [ -d "$PROJECT_DIR/schemas/custom" ]; then
                echo "Custom schema directory created: $PROJECT_DIR/schema/custom"
                echo "  Note: Add custom schemas to $PROJECT_DIR/schemas/custom/queries.json"
            fi
            ;;

        CROWCPP)
            echo "Copying CROWCPP template structure..."

            # Copy everything except src/include directory
            for item in "$TEMPLATE_SUBDIR"/*; do
                item_name=$(basename "$item")
                if [ "$item_name" != "src" ]; then
                    cp -r "$item" "$PROJECT_DIR/"
                    echo "Copied $item_name"
                fi
            done

            # Handle src directory separately
            if [ -d "$TEMPLATE_SUBDIR/src" ]; then
                # Copy everything in src except include
                for item in "$TEMPLATE_SUBDIR/src"/*; do
                    item_name=$(basename "$item")
                    if [ "$item_name" != "include" ]; then
                        mkdir -p "$PROJECT_DIR/src"
                        cp -r "$item" "$PROJECT_DIR/src/"
                        echo "Copied src/$item_name"
                    fi
                done

                # Handle src/include directory
                if [ -d "$TEMPLATE_SUBDIR/src/include" ]; then
                    INCLUDE_SRC="$TEMPLATE_SUBDIR/src/include"
                    INCLUDE_DEST="$PROJECT_DIR/src/include"
                    mkdir -p "$INCLUDE_DEST"

                    # Copy files directly in include/ (like extension_config.h.in)
                    find "$INCLUDE_SRC" -maxdepth 1 -type f -exec cp {} "$INCLUDE_DEST/" \;

                    # Copy Cache directory
                    if [ -d "$INCLUDE_SRC/Cache" ]; then
                        cp -r "$INCLUDE_SRC/Cache" "$INCLUDE_DEST/"
                        echo "Copied src/include/Cache"
                    fi

                    # Copy only the selected database directory
                    if [ -d "$INCLUDE_SRC/Database/$DB_DIR" ]; then
                        mkdir -p "$INCLUDE_DEST/Database"
                        cp -r "$INCLUDE_SRC/Database/$DB_DIR" "$INCLUDE_DEST/Database/"
                        echo "Copied src/include/Database/$DB_DIR"
                    else
                        echo "Warning: Database template for $DB_DIR not found"
                    fi
                fi
            fi

            # Check if queries/custom directory were copied
            if [ -d "$PROJECT_DIR/queries/custom" ]; then
                echo "Custom query definitions directory created: $PROJECT_DIR/queries/custom"
                echo "  Note: Add custom queries to $PROJECT_DIR/queries/custom/queries.yaml"
            fi

            # Check if schemas/custom directory were copied
            if [ -d "$PROJECT_DIR/schemas/custom" ]; then
                echo "Custom schema directory created: $PROJECT_DIR/schema/custom"
                echo "  Note: Add custom schemas to $PROJECT_DIR/schemas/custom/queries.json"
            fi
            ;;
    esac

    echo ""
    echo "Project initialization completed successfully!"
    echo "  Source directory: $SOURCE_DIR"

    if [ "$PROJECT_TYPE" = "BINDING" ] && [ "$BINDING_LANGUAGE" = "NODEJS" ]; then
        echo "  TypeScript definitions: $PROJECT_DIR/typescript/types/${PROJECT_NAME}.d.ts"
        echo "  TypeScript test file: $PROJECT_DIR/typescript/types/test-types.ts"
        echo "  TypeScript config: $PROJECT_DIR/typescript/tsconfig.json"
    fi

    if [ -d "$PROJECT_DIR/queries" ]; then
        echo "  Custom queries directory: $PROJECT_DIR/queries/custom"
        echo "  Custom schemas directory: $PROJECT_DIR/schemas/custom"
        echo ""
        echo "To add custom queries:"
        echo "  1. Edit $PROJECT_DIR/queries/custom/queries.yaml"
        echo "  2. Add corresponding schema to $PROJECT_DIR/schemas/custom/queries.json"
        echo "  3. Re-run CMake to regenerate boilerplate code"
    fi
}

# Variables
INIT_FLAG=false
PURGE_FLAG=false
#SOURCE_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
#TEMPLATE_DIR="$SOURCE_DIR/template"

#SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
#SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
#TEMPLATE_DIR="$(cd "$SOURCE_DIR" && pwd)/template"
#PROJECT_DIR="$(cd "$SOURCE_DIR/.." && pwd)"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR/.."
PROJECT_DIR="$SOURCE_DIR/.."
TEMPLATE_DIR="$SOURCE_DIR/template"

# At least 1 argument required
[ $# -lt 1 ] && usage
PROJECT_NAME="${2:-""}"

case "$1" in
    --init)
        INIT_FLAG=true
        ;;
    --purge)
        PURGE_FLAG=true
        ;;
    *)
        echo "Error: unknown option '$1'"
        usage
        ;;
esac




# Check for mutually exclusive flags
if [ "$INIT_FLAG" = true ] && [ "$PURGE_FLAG" = true ]; then
    echo "Error: Options --init and --purge are mutually exclusive"
    usage
fi

# Execute based on flag
if [ "$PURGE_FLAG" = true ]; then
    purge_project
    exit $?
elif [ "$INIT_FLAG" = true ]; then
    init_project
    exit 0
else
    echo "Error: No option specified"
    usage
fi

# vim: set tabstop=2 shiftwidth=2 expandtab colorcolumn=121 :
