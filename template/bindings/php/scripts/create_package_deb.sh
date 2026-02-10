#!/bin/bash

# Get database backend and project name from command line
DB_BACKEND=${1:-"MYSQL"}
PROJECT_NAME=${2:-"caos"}
PROJECT_NAME_SANITIZED=$(echo "$PROJECT_NAME" | tr '_' '-')
DB_BACKEND_LOWER=$(echo "$DB_BACKEND" | tr '[:upper:]' '[:lower:]')

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../" && pwd)"
#BUILD_DIR="$PROJECT_ROOT/CAOSDBA/build/release"
BUILD_DIR="$PROJECT_ROOT/build/release"
DIST_DIR="$PROJECT_ROOT/dist"
REPO_DIR="$DIST_DIR/repositories/${PROJECT_NAME}/php"
PACKAGE_BASE_NAME="${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}"

if [ -f "$BUILD_DIR/build_counter.txt" ]; then
    CAOS_BUILD_COUNTER=$(cat "$BUILD_DIR/build_counter.txt")
    VERSION="1.0.0+${CAOS_BUILD_COUNTER}"
else
    VERSION="1.0.0+1"
    echo "WARNING: build_counter.txt not found, using default version"
fi

echo "Building DEB packages for ${PROJECT_NAME} PHP extension"
echo "Project: $PROJECT_NAME"
echo "Version: $VERSION"
echo "Database backend: $DB_BACKEND_LOWER"
echo "Repository directory: $REPO_DIR"

PHP_VERSIONS="8.0 8.1 8.2 8.3 8.4"

get_php_api_version() {
    local php_version=$1
    case $php_version in
        "8.0") echo "20200930" ;;
        "8.1") echo "20210902" ;;
        "8.2") echo "20220829" ;;
        "8.3") echo "20230831" ;;
        "8.4") echo "20240829" ;;
        *) echo "20230831" ;;
    esac
}

calculate_php_paths() {
    local php_version=$1
    PHP_API=$(get_php_api_version "$php_version")
    PHP_EXT_DIR="/usr/lib/php/$PHP_API"
    MODS_AVAILABLE_DIR="/etc/php/$php_version/mods-available"
}

check_prerequisites() {
    # Check if extension files exist in nested repository directory
    local so_pattern="${PROJECT_NAME}-${DB_BACKEND_LOWER}-*.so"
    local ini_pattern="${PROJECT_NAME}-${DB_BACKEND_LOWER}-*.ini"

    local so_files=$(find "$REPO_DIR" -name "$so_pattern" -type f 2>/dev/null)
    local ini_files=$(find "$REPO_DIR" -name "$ini_pattern" -type f 2>/dev/null)

    if [ -z "$so_files" ]; then
        echo "ERROR: No PHP extension files found in $REPO_DIR"
        echo "Expected pattern: $so_pattern"
        echo "Please build the extension first:"
        echo "  cmake --build build/release --target ${PROJECT_NAME}"
        exit 1
    fi

    if [ -z "$ini_files" ]; then
        echo "ERROR: No PHP ini files found in $REPO_DIR"
        echo "Expected pattern: $ini_pattern"
        exit 1
    fi

    if ! command -v dpkg-deb >/dev/null 2>&1; then
        echo "ERROR: dpkg-deb not found. Install dpkg-dev package"
        exit 1
    fi

    echo "Found PHP extension files in nested repository directory"
}

create_structure() {
    local deb_dir=$1
    rm -rf "$deb_dir"
    mkdir -p "$deb_dir/$PHP_EXT_DIR"
    mkdir -p "$deb_dir/$MODS_AVAILABLE_DIR"
    mkdir -p "$deb_dir/DEBIAN"
}

copy_files() {
    local deb_dir=$1
    local php_version=$2

    # Find the latest .so and .ini files for this build
    local latest_so=$(find "$REPO_DIR" -name "${PROJECT_NAME}-${DB_BACKEND_LOWER}-*.so" -type f | sort -V | tail -1)
    local latest_ini=$(find "$REPO_DIR" -name "${PROJECT_NAME}-${DB_BACKEND_LOWER}-*.ini" -type f | sort -V | tail -1)

    if [ -z "$latest_so" ] || [ -z "$latest_ini" ]; then
        echo "ERROR: Could not find extension files for PHP $php_version"
        return 1
    fi

    # Copy the extension .so file
    cp "$latest_so" "$deb_dir/$PHP_EXT_DIR/${PROJECT_NAME}.so"

    # Copy the .ini configuration file
    cp "$latest_ini" "$deb_dir/$MODS_AVAILABLE_DIR/${PROJECT_NAME}.ini"

    return 0
}

create_postinst() {
    local deb_dir=$1
    local php_version=$2
    cat > "$deb_dir/DEBIAN/postinst" << EOF
#!/bin/bash
set -e

PHP_VERSION="$php_version"
PHP_EXT_DIR="$PHP_EXT_DIR"
MODS_AVAILABLE_DIR="$MODS_AVAILABLE_DIR"
PROJECT_NAME="$PROJECT_NAME"

echo ""
echo "Processing ${PROJECT_NAME} PHP extension for PHP \$PHP_VERSION"
echo ""

# Check if this PHP version is installed on target system
if [ ! -d "/etc/php/\$PHP_VERSION" ]; then
    echo "INFO: PHP \$PHP_VERSION not installed on this system"
    echo "Extension will be automatically enabled if PHP \$PHP_VERSION is installed later"
    exit 0
fi

echo "PHP \$PHP_VERSION is installed - enabling extension"

# Set owner and permissions
chown root:root "\$PHP_EXT_DIR/\$PROJECT_NAME.so" 2>/dev/null || true
chmod 644 "\$PHP_EXT_DIR/\$PROJECT_NAME.so" 2>/dev/null || true
chown root:root "\$MODS_AVAILABLE_DIR/\$PROJECT_NAME.ini" 2>/dev/null || true
chmod 644 "\$MODS_AVAILABLE_DIR/\$PROJECT_NAME.ini" 2>/dev/null || true

# Enable the extension
if command -v phpenmod >/dev/null 2>&1; then
    echo "Enabling \$PROJECT_NAME extension for PHP \$PHP_VERSION"
    phpenmod -v \$PHP_VERSION -s cli \$PROJECT_NAME 2>/dev/null && echo "Enabled for CLI" || echo "WARNING: Could not enable for CLI"
    phpenmod -v \$PHP_VERSION -s fpm \$PROJECT_NAME 2>/dev/null && echo "Enabled for FPM" || echo "WARNING: Could not enable for FPM"
    phpenmod -v \$PHP_VERSION -s apache2 \$PROJECT_NAME 2>/dev/null && echo "Enabled for Apache" || echo "WARNING: Could not enable for Apache"

    # Reload services if running
    systemctl reload "php\$PHP_VERSION-fpm" 2>/dev/null || true
    systemctl reload "apache2" 2>/dev/null || true
else
    echo "ERROR: phpenmod not found. Please install php\$PHP_VERSION-common"
fi

echo ""
echo "\$PROJECT_NAME PHP extension for PHP \$PHP_VERSION ready"
echo "TIP: Run: php\$PHP_VERSION -m | grep \$PROJECT_NAME  to verify"
echo ""
EOF
    chmod +x "$deb_dir/DEBIAN/postinst"
}

create_prerm() {
    local deb_dir=$1
    local php_version=$2
    cat > "$deb_dir/DEBIAN/prerm" << EOF
#!/bin/bash
set -e

PHP_VERSION="$php_version"
PHP_EXT_DIR="$PHP_EXT_DIR"
MODS_AVAILABLE_DIR="$MODS_AVAILABLE_DIR"
PROJECT_NAME="$PROJECT_NAME"

echo ""
echo "Disabling \$PROJECT_NAME PHP extension for PHP \$PHP_VERSION before removal"
echo ""

# Disable extension for all SAPI if this PHP version is installed
if [ -d "/etc/php/\$PHP_VERSION" ] && command -v phpdismod >/dev/null 2>&1; then
    echo "Disabling extension for all SAPI"
    phpdismod -v \$PHP_VERSION -s cli \$PROJECT_NAME 2>/dev/null && echo "Disabled for CLI" || echo "WARNING: Could not disable for CLI"
    phpdismod -v \$PHP_VERSION -s fpm \$PROJECT_NAME 2>/dev/null && echo "Disabled for FPM" || echo "WARNING: Could not disable for FPM"
    phpdismod -v \$PHP_VERSION -s apache2 \$PROJECT_NAME 2>/dev/null && echo "Disabled for Apache" || echo "WARNING: Could not disable for Apache"

    # Reload services if running
    if systemctl is-active --quiet "php\$PHP_VERSION-fpm" 2>/dev/null; then
        echo "Reloading PHP-FPM"
        systemctl reload "php\$PHP_VERSION-fpm" 2>/dev/null || true
    fi

    if systemctl is-active --quiet "apache2" 2>/dev/null; then
        echo "Reloading Apache"
        systemctl reload "apache2" 2>/dev/null || true
    fi

    echo "\$PROJECT_NAME extension for PHP \$PHP_VERSION disabled before removal"
else
    echo "INFO: PHP \$PHP_VERSION not installed or phpdismod not available"
fi

echo ""
EOF
    chmod +x "$deb_dir/DEBIAN/prerm"
}

create_control() {
    local deb_dir=$1
    local package_name=$2
    local php_version=$3
    local architecture="amd64"

    local common_depends="php${php_version}-common, ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}"

    local conflicts_clause=""
    case "${DB_BACKEND_LOWER}" in
        "mysql")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-php-mariadb-${php_version}, ${PROJECT_NAME_SANITIZED}-php-postgresql-${php_version}"
            ;;
        "mariadb")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-php-mysql-${php_version}, ${PROJECT_NAME_SANITIZED}-php-postgresql-${php_version}"
            ;;
        "postgresql")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-php-mysql-${php_version}, ${PROJECT_NAME_SANITIZED}-php-mariadb-${php_version}"
            ;;
        *)
            ;;
    esac

    cat > "$deb_dir/DEBIAN/control" << EOF
Package: $package_name
Version: $VERSION
Architecture: $architecture
Maintainer: CAOS Development Team
Description: ${PROJECT_NAME} - Cache App On Steroids extension for PHP $php_version with $DB_BACKEND_LOWER backend
 High-performance database access extension (Cache App On Steroids).
 Supports: CLI, FPM, Apache, Nginx for PHP $php_version with $DB_BACKEND_LOWER backend.
Depends: $common_depends
Recommends: php$php_version-fpm | libapache2-mod-php$php_version
Conflicts: $conflicts_clause
Section: php
Priority: optional
EOF
}

build_single_package() {
    local php_version=$1
    local deb_dir="$BUILD_DIR/deb-staging-php$php_version"

    calculate_php_paths "$php_version"
    PHP_VERSION="$php_version"

    local package_name="${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-$php_version"

    echo "Building package: $package_name"

    create_structure "$deb_dir"
    if ! copy_files "$deb_dir" "$php_version"; then
        echo "ERROR: Failed to copy files for PHP $php_version"
        return 1
    fi
    create_postinst "$deb_dir" "$php_version"
    create_prerm "$deb_dir" "$php_version"
    create_control "$deb_dir" "$package_name" "$php_version"

    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${package_name}_${VERSION}_amd64.deb" >/dev/null 2>&1

    rm -rf "$deb_dir"

    echo "Built: ${package_name}_${VERSION}_amd64.deb"
    return 0
}

create_meta_package() {
    local deb_dir="$BUILD_DIR/deb-staging-meta"
    mkdir -p "$deb_dir/DEBIAN"

    echo "Creating meta-package with version: $VERSION"
    echo "Build counter: $CAOS_BUILD_COUNTER"

    # No dependencies for meta-package - libraries are statically linked
    local meta_common_depends=""

    # Meta-package recommends specific packages instead of depending on them
    local recommends_clause=""
    for version in $PHP_VERSIONS; do
        pkg="${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-$version"
        recommends_clause="$recommends_clause$pkg (>= $VERSION), "
    done
    recommends_clause="${recommends_clause%, }"

    # Generate conflicts for meta-package
    local meta_conflicts_clause=""
    case "${DB_BACKEND_LOWER}" in
        "mysql")
            meta_conflicts_clause="${PROJECT_NAME_SANITIZED}-php-mariadb, ${PROJECT_NAME_SANITIZED}-php-postgresql"
            ;;
        "mariadb")
            meta_conflicts_clause="${PROJECT_NAME_SANITIZED}-php-mysql, ${PROJECT_NAME_SANITIZED}-php-postgresql"
            ;;
        "postgresql")
            meta_conflicts_clause="${PROJECT_NAME_SANITIZED}-php-mysql, ${PROJECT_NAME_SANITIZED}-php-mariadb"
            ;;
        *)
            ;;
    esac

    cat > "$deb_dir/DEBIAN/control" << EOF
Package: ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}
Version: $VERSION
Architecture: all
Maintainer: Alessandro Bianco <mydevhero@gmail.com>
Description: ${PROJECT_NAME} - Cache App On Steroids PHP extension with $DB_BACKEND_LOWER backend (metapackage)
 High-performance database access extension (Cache App On Steroids).
 This metapackage will install the appropriate version for your PHP.
 Removing this metapackage will also remove the specific PHP version packages.
Recommends: $recommends_clause
Conflicts: $meta_conflicts_clause
Section: php
Priority: optional
EOF

    echo "Building meta-package deb file..."
    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-${VERSION}_all.deb"

    # Verify the file was created
    if [ -f "$BUILD_DIR/${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-${VERSION}_all.deb" ]; then
        echo "DEBUG: Meta-package successfully created: ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-${VERSION}_all.deb"
        ls -la "$BUILD_DIR/${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-${VERSION}_all.deb"
    else
        echo "DEBUG: ERROR - Meta-package file was not created!"
    fi

    rm -rf "$deb_dir"
    echo "Meta-package built: ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-${VERSION}_all.deb"
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
            echo "\$package_name optional php" >> "\$OVERRIDE_FILE"
        fi
    fi
done

cd "\$REPO_DIR" || exit 1

rm -f Release.gpg InRelease

dpkg-scanpackages --multiversion . "\$OVERRIDE_FILE" > Packages
gzip -k -f Packages

cat > Release << RELEASE_CONTENT
Origin: ${PROJECT_NAME} PHP Repository
Label: ${PROJECT_NAME} PHP
Suite: stable
Codename: ${PROJECT_NAME}-php
Version: 1.0
Architectures: amd64 all
Components: main
Description: ${PROJECT_NAME} - CAOS PHP extension
Date: \$(date -Ru)
MD5Sum:
 \$(md5sum Packages | cut -d' ' -f1) \$(stat -c %s Packages) Packages
 \$(md5sum Packages.gz | cut -d' ' -f1) \$(stat -c %s Packages.gz) Packages.gz
SHA256:
 \$(sha256sum Packages | cut -d' ' -f1) \$(stat -c %s Packages) Packages
 \$(sha256sum Packages.gz | cut -d' ' -f1) \$(stat -c %s Packages.gz) Packages.gz
RELEASE_CONTENT

echo "PHP repository index updated"
EOF
    chmod +x "$REPO_DIR/update-repo.sh"

    cat > "$REPO_DIR/setup-apt-source.sh" << EOF
#!/bin/bash

REPO_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
APT_SOURCE_FILE="/etc/apt/sources.list.d/${PROJECT_NAME}-php-local.list"

if [ "\$EUID" -ne 0 ]; then
    echo "ERROR: This script must be run as root"
    exit 1
fi

if [ ! -d "\$REPO_DIR" ] || [ ! -f "\$REPO_DIR/Packages.gz" ]; then
    echo "ERROR: PHP repository not found or not indexed"
    exit 1
fi

# Clean up any existing PHP repository configurations
echo "Cleaning up existing PHP repository configurations..."
for file in /etc/apt/sources.list.d/*${PROJECT_NAME}*php*.list; do
    if [ -f "\$file" ]; then
        echo "  Removing: \$(basename "\$file")"
        rm -f "\$file"
    fi
done

echo "deb [trusted=yes] file:\$REPO_DIR ./" > "\$APT_SOURCE_FILE"

apt update

echo "PHP repository configured"
echo "Install: sudo apt install ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}"
EOF
    chmod +x "$REPO_DIR/setup-apt-source.sh"
}

move_packages_to_repository() {
    mkdir -p "$REPO_DIR"
    mv "$BUILD_DIR"/*.deb "$REPO_DIR/" 2>/dev/null || true
}

main() {
    echo "Starting DEB package creation for $PROJECT_NAME PHP extension..."

    check_prerequisites

    local built_count=0
    for version in $PHP_VERSIONS; do
        echo "=== PROCESSING PHP $version ==="
        if build_single_package "$version"; then
            built_count=$((built_count + 1))
            echo "SUCCESS: PHP $version package built"
        else
            echo "WARNING: PHP $version package skipped"
        fi
        echo "=== COMPLETED PHP $version ==="
        echo ""
    done

    if [ $built_count -gt 0 ]; then
         create_meta_package
         move_packages_to_repository
         create_repository_scripts

         echo "PHP multi-version packaging completed"
         echo "Built $built_count individual packages + 1 meta-package"
         echo "PHP repository created in: $REPO_DIR"
         exit 0
     else
         echo "ERROR: No PHP packages were built successfully"
         exit 1
     fi
}

main "$@"
