#!/bin/bash

DB_BACKEND=${1:-"MYSQL"}
PROJECT_NAME=${2:-"caos"}
PROJECT_NAME_SANITIZED=$(echo "$PROJECT_NAME" | tr '_' '-')
DB_BACKEND_LOWER=$(echo "$DB_BACKEND" | tr '[:upper:]' '[:lower:]')

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../" && pwd)"
#BUILD_DIR="$PROJECT_ROOT/CAOSDBA/build/release"
BUILD_DIR="$PROJECT_ROOT/build/release"
DIST_DIR="$PROJECT_ROOT/dist"
REPO_DIR="$DIST_DIR/repositories/${PROJECT_NAME}/python"

if [ -f "$BUILD_DIR/build_counter.txt" ]; then
    CAOS_BUILD_COUNTER=$(cat "$BUILD_DIR/build_counter.txt")
    VERSION="1.0.0+${CAOS_BUILD_COUNTER}"
else
    VERSION="1.0.0+1"
    echo "WARNING: Using default version"
fi

extract_python_version() {
    local filename=$1
    local basename=$(basename "$filename")

    # Pattern: my_app.cpython-312-x86_64-linux-gnu.so
    if [[ "$basename" =~ \.cpython-([0-9]+) ]]; then
        local version="${BASH_REMATCH[1]}"
        local major="${version:0:1}"
        local minor="${version:1:2}"
        echo "${major}.${minor}"
    elif [[ "$basename" =~ \.cpython-([0-9])([0-9]+)- ]]; then
        local major="${BASH_REMATCH[1]}"
        local minor="${BASH_REMATCH[2]}"
        echo "${major}.${minor}"
    else
        echo ""
    fi
}

extract_build_number() {
    local filename=$1
    local basename=$(basename "$filename")

    if [ -f "$BUILD_DIR/build_counter.txt" ]; then
        cat "$BUILD_DIR/build_counter.txt"
    else
        echo "1"
    fi
}

find_python_extension_files() {
    local pattern="${PROJECT_NAME}.cpython-*.so"
    find "$DIST_DIR" -name "$pattern" -type f 2>/dev/null
}

calculate_python_paths() {
    local python_version=$1
    PYTHON_SITE_DIR="/usr/lib/python3/dist-packages"
    PYTHON_PACKAGE_DIR="${PYTHON_SITE_DIR}/${PROJECT_NAME}"
}

check_prerequisites() {
    # Check if Python repository directory exists
    if [ ! -d "$REPO_DIR" ]; then
        echo "ERROR: Python repository directory not found: $REPO_DIR"
        echo "Expected: $DIST_DIR/repositories/${PROJECT_NAME}/python/"
        exit 1
    fi

    # Check for .so files in the Python repository directory
    local ext_files=$(find "$REPO_DIR" -name "${PROJECT_NAME}.cpython-*.so" -type f 2>/dev/null)

    if [ -z "$ext_files" ]; then
        echo "ERROR: No Python extension files found in $REPO_DIR"
        echo "Expected pattern: ${PROJECT_NAME}.cpython-*.so"
        echo "Actual files in $REPO_DIR:"
        find "$REPO_DIR" -name "*.so" -type f 2>/dev/null || echo "No .so files found"
        exit 1
    fi

    local file_count=$(echo "$ext_files" | wc -l)

    if [ "$file_count" -eq 0 ]; then
        echo "ERROR: No Python extension files found"
        exit 1
    fi

    if ! command -v dpkg-deb >/dev/null 2>&1; then
        echo "ERROR: dpkg-deb not found. Install dpkg-dev"
        exit 1
    fi
}

create_structure() {
    local deb_dir=$1
    local python_version=$2
    rm -rf "$deb_dir"
    mkdir -p "$deb_dir/$PYTHON_PACKAGE_DIR"
    mkdir -p "$deb_dir/DEBIAN"
}

copy_files() {
    local deb_dir=$1
    local source_file=$2
    local python_version=$3

    local so_filename=$(basename "$source_file")
    cp "$source_file" "$deb_dir/$PYTHON_PACKAGE_DIR/"

    # Only create __init__.py if it doesn't exist
    if [ ! -f "$deb_dir/$PYTHON_PACKAGE_DIR/__init__.py" ]; then
      cat > "$deb_dir/$PYTHON_PACKAGE_DIR/__init__.py" << 'EOF'
"""
${PROJECT_NAME} - CAOS Native Python Bindings
==============================================

Native extension for CAOS database operations.
Backend: ${DB_BACKEND} (${DB_BACKEND_LOWER})

Usage:
    import ${PROJECT_NAME}
"""

import importlib.util
import sys
import os
import glob

def _load_correct_module():
    """Load the correct .so module for current Python version."""
    package_dir = os.path.dirname(__file__)

    # Find all .so files in this directory
    so_files = glob.glob(os.path.join(package_dir, "*.cpython-*.so"))

    if not so_files:
        raise ImportError(f"No native module found in {package_dir}")

    # Get current Python version suffix (e.g., "312" for Python 3.12)
    current_suffix = f"cpython-{sys.version_info.major}{sys.version_info.minor}"

    # Try to find exact match first
    for so_file in so_files:
        if current_suffix in so_file:
            return so_file

    # If no exact match, use first available (fallback)
    return so_files[0]

# Load the native module
module_path = _load_correct_module()
module_filename = os.path.basename(module_path)

spec = importlib.util.spec_from_file_location('${PROJECT_NAME}', module_path)
module = importlib.util.module_from_spec(spec)
sys.modules[__name__] = module
spec.loader.exec_module(module)

# Expose all public functions from native module
for attr in dir(module):
    if not attr.startswith('_'):
        globals()[attr] = getattr(module, attr)

__version__ = '${VERSION}'
__backend__ = '${DB_BACKEND_LOWER}'
__python_version__ = f"{sys.version_info.major}.{sys.version_info.minor}"

def get_build_info():
    """Return build information."""
    return {
        'module': '${PROJECT_NAME}',
        'version': __version__,
        'backend': __backend__,
        'python_version': __python_version__,
        'loaded_module': os.path.basename(module_path)
    }
EOF
    fi
}

create_postinst() {
    local deb_dir=$1
    local python_version=$2

    cat > "$deb_dir/DEBIAN/postinst" << 'POSTINST_EOF'
#!/bin/bash
set -e

# These variables should be defined when the package is installed
# They come from the package name or could be hardcoded
PACKAGE_NAME="my_app"  # Or extract from actual package name
DB_BACKEND="POSTGRESQL"  # Or extract from package name
DB_BACKEND_LOWER="postgresql"  # Or extract from package name

PYTHON_PACKAGE_DIR="/usr/lib/python3/dist-packages/${PACKAGE_NAME}"

echo "Configuring ${PACKAGE_NAME} Python module..."

if [ -d "$PYTHON_PACKAGE_DIR" ] && [ -f "$PYTHON_PACKAGE_DIR/__init__.py" ]; then
    echo "  Updating __init__.py with correct module names..."

    # Escape special characters for sed
    PACKAGE_NAME_ESC=$(printf '%s\n' "$PACKAGE_NAME" | sed 's/[&/\]/\\&/g')
    DB_BACKEND_ESC=$(printf '%s\n' "$DB_BACKEND" | sed 's/[&/\]/\\&/g')
    DB_BACKEND_LOWER_ESC=$(printf '%s\n' "$DB_BACKEND_LOWER" | sed 's/[&/\]/\\&/g')

    # Fix module name (remove '_native' suffix)
    sed -i "s/'${PACKAGE_NAME_ESC}_native'/'${PACKAGE_NAME_ESC}'/g" "$PYTHON_PACKAGE_DIR/__init__.py"

    # Replace other variables
    sed -i "s/\${PROJECT_NAME}/${PACKAGE_NAME_ESC}/g" "$PYTHON_PACKAGE_DIR/__init__.py"
    sed -i "s/\${DB_BACKEND}/${DB_BACKEND_ESC}/g" "$PYTHON_PACKAGE_DIR/__init__.py"
    sed -i "s/\${DB_BACKEND_LOWER}/${DB_BACKEND_LOWER_ESC}/g" "$PYTHON_PACKAGE_DIR/__init__.py"

    # Fix any escaped quotes (if they exist)
    sed -i 's/\\"\\"\\"/"""/g' "$PYTHON_PACKAGE_DIR/__init__.py"
    sed -i 's/\"\"\"/"""/g' "$PYTHON_PACKAGE_DIR/__init__.py"

    echo "  __init__.py updated successfully"
else
    echo "  WARNING: Python package directory not found: $PYTHON_PACKAGE_DIR"
fi

echo "Installation completed."
POSTINST_EOF

    chmod +x "$deb_dir/DEBIAN/postinst"
}

create_prerm() {
    local deb_dir=$1
    local python_version=$2
    cat > "$deb_dir/DEBIAN/prerm" << EOF
#!/bin/bash
set -e
EOF
    chmod +x "$deb_dir/DEBIAN/prerm"
}

create_control() {
    local deb_dir=$1
    local package_name=$2
    local python_version=$3
    local architecture="amd64"

    # Python specific package DEPENDS on meta-package
    local python_dep="python${python_version} | python${python_version}-minimal, ${PROJECT_NAME_SANITIZED}-python-${DB_BACKEND_LOWER}"

    local conflicts_clause=""
    case "${DB_BACKEND_LOWER}" in
        "mysql")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-python-mariadb-${python_version}, ${PROJECT_NAME_SANITIZED}-python-postgresql-${python_version}"
            ;;
        "mariadb")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-python-mysql-${python_version}, ${PROJECT_NAME_SANITIZED}-python-postgresql-${python_version}"
            ;;
        "postgresql")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-python-mysql-${python_version}, ${PROJECT_NAME_SANITIZED}-python-mariadb-${python_version}"
            ;;
        *)
            ;;
    esac

    cat > "$deb_dir/DEBIAN/control" << EOF
Package: $package_name
Version: $VERSION
Architecture: $architecture
Maintainer: CAOS Development Team
Description: ${PROJECT_NAME} - CAOS extension for Python $python_version with $DB_BACKEND_LOWER backend
Depends: $python_dep
Conflicts: $conflicts_clause
Section: python
Priority: optional
EOF
}

build_single_package() {
    local source_file=$1
    local python_version=$2
    local build_number=$3

    calculate_python_paths "$python_version"

    local package_name="${PROJECT_NAME_SANITIZED}-python-${DB_BACKEND_LOWER}-${python_version}"
    local deb_dir="$BUILD_DIR/deb-python-${python_version}"

    create_structure "$deb_dir" "$python_version"
    copy_files "$deb_dir" "$source_file" "$python_version"

    create_postinst "$deb_dir" "$python_version"
    create_prerm "$deb_dir" "$python_version"
    create_control "$deb_dir" "$package_name" "$python_version"

    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${package_name}_${VERSION}_amd64.deb" >/dev/null 2>&1

    rm -rf "$deb_dir"

    echo "Built: ${package_name}_${VERSION}_amd64.deb"
    return 0
}

create_meta_package() {
    local deb_dir="$BUILD_DIR/deb-meta-python"
    mkdir -p "$deb_dir/DEBIAN"

    local recommends_clause=""
    for pkg_file in "$BUILD_DIR"/*-python-${DB_BACKEND_LOWER}-*_${VERSION}_amd64.deb; do
        if [ -f "$pkg_file" ]; then
            pkg_name=$(basename "$pkg_file" | cut -d'_' -f1)
            recommends_clause="$recommends_clause$pkg_name (>= $VERSION), "
        fi
    done
    recommends_clause="${recommends_clause%, }"

    local meta_conflicts_clause=""
    case "${DB_BACKEND_LOWER}" in
        "mysql")
            meta_conflicts_clause="${PROJECT_NAME_SANITIZED}-python-mariadb, ${PROJECT_NAME_SANITIZED}-python-postgresql"
            ;;
        "mariadb")
            meta_conflicts_clause="${PROJECT_NAME_SANITIZED}-python-mysql, ${PROJECT_NAME_SANITIZED}-python-postgresql"
            ;;
        "postgresql")
            meta_conflicts_clause="${PROJECT_NAME_SANITIZED}-python-mysql, ${PROJECT_NAME_SANITIZED}-python-mariadb"
            ;;
        *)
            ;;
    esac

    cat > "$deb_dir/DEBIAN/control" << EOF
Package: ${PROJECT_NAME_SANITIZED}-python-${DB_BACKEND_LOWER}
Version: $VERSION
Architecture: all
Maintainer: CAOS Development Team
Description: ${PROJECT_NAME} - CAOS Python extension with $DB_BACKEND_LOWER backend (metapackage)
 This metapackage will install the appropriate Python version packages.
Recommends: $recommends_clause
Conflicts: $meta_conflicts_clause
Section: python
Priority: optional
EOF

    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${PROJECT_NAME_SANITIZED}-python-${DB_BACKEND_LOWER}_${VERSION}_all.deb" >/dev/null 2>&1

    rm -rf "$deb_dir"
    echo "Built meta-package"
}

create_repository_scripts() {
    mkdir -p "$REPO_DIR"

    cat > "$REPO_DIR/update-repo.sh" << EOF
#!/bin/bash

REPO_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
CONF_DIR="\$REPO_DIR/conf"
OVERRIDE_FILE="\$CONF_DIR/override"

mkdir -p "\$CONF_DIR"

> "\$OVERRIDE_FILE"
for deb_file in "\$REPO_DIR"/*.deb; do
    if [ -f "\$deb_file" ]; then
        package_name=\$(dpkg-deb -f "\$deb_file" Package 2>/dev/null || basename "\$deb_file" | cut -d'_' -f1)
        if [ -n "\$package_name" ]; then
            echo "\$package_name optional python" >> "\$OVERRIDE_FILE"
        fi
    fi
done

cd "\$REPO_DIR" || exit 1

rm -f Release.gpg InRelease

dpkg-scanpackages --multiversion . "\$OVERRIDE_FILE" > Packages
gzip -k -f Packages

cat > Release << RELEASE_CONTENT
Origin: ${PROJECT_NAME} Python Repository
Label: ${PROJECT_NAME}
Suite: stable
Codename: ${PROJECT_NAME}
Version: 1.0
Architectures: amd64 all
Components: main
Description: ${PROJECT_NAME} - CAOS Python extension
Date: \$(date -Ru)
MD5Sum:
 \$(md5sum Packages | cut -d' ' -f1) \$(stat -c %s Packages) Packages
 \$(md5sum Packages.gz | cut -d' ' -f1) \$(stat -c %s Packages.gz) Packages.gz
SHA256:
 \$(sha256sum Packages | cut -d' ' -f1) \$(stat -c %s Packages) Packages
 \$(sha256sum Packages.gz | cut -d' ' -f1) \$(stat -c %s Packages.gz) Packages.gz
RELEASE_CONTENT

echo "Repository index updated"
EOF
    chmod +x "$REPO_DIR/update-repo.sh"

    cat > "$REPO_DIR/setup-apt-source.sh" << EOF
#!/bin/bash

REPO_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
APT_SOURCE_FILE="/etc/apt/sources.list.d/${PROJECT_NAME}-python-local.list"

if [ "\$EUID" -ne 0 ]; then
    echo "ERROR: This script must be run as root"
    exit 1
fi

if [ ! -d "\$REPO_DIR" ] || [ ! -f "\$REPO_DIR/Packages.gz" ]; then
    echo "ERROR: Repository not found or not indexed"
    exit 1
fi

# Clean up any existing conflicting source files
echo "Cleaning up existing repository configurations..."
for file in /etc/apt/sources.list.d/*${PROJECT_NAME}*.list; do
    if [ -f "\$file" ]; then
        echo "  Removing: \$(basename "\$file")"
        rm -f "\$file"
    fi
done

echo "deb [trusted=yes] file:\$REPO_DIR ./" > "\$APT_SOURCE_FILE"

apt update

echo "Install: sudo apt install ${PROJECT_NAME_SANITIZED}-python-${DB_BACKEND_LOWER}"
EOF
    chmod +x "$REPO_DIR/setup-apt-source.sh"
}

move_packages_to_repository() {
    mkdir -p "$REPO_DIR"
    mv "$BUILD_DIR"/*.deb "$REPO_DIR/" 2>/dev/null || true
}

main() {
    check_prerequisites

    # Look for extension files in the Python repository directory
    local ext_files=$(find "$REPO_DIR" -name "${PROJECT_NAME}.cpython-*.so" -type f 2>/dev/null)
    local built_count=0

    while IFS= read -r source_file; do
        if [ -z "$source_file" ]; then
            continue
        fi

        local python_version=$(extract_python_version "$source_file")
        local build_number=$(extract_build_number "$source_file")

        if [ -z "$python_version" ]; then
            continue
        fi

        if build_single_package "$source_file" "$python_version" "$build_number"; then
            built_count=$((built_count + 1))
        fi
    done <<< "$ext_files"

    if [ "$built_count" -gt 0 ]; then
        create_meta_package
        move_packages_to_repository
        create_repository_scripts
        echo "Packaging completed: $built_count packages"
        exit 0
    else
        echo "ERROR: No packages built"
        exit 1
    fi
}

main "$@"
