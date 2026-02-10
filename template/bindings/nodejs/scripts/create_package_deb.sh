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
REPO_DIR="$DIST_DIR/repositories/${PROJECT_NAME}/nodejs"
PACKAGE_NAME_BASE="${PROJECT_NAME_SANITIZED}-nodejs-${DB_BACKEND_LOWER}"

if [ -f "$BUILD_DIR/build_counter.txt" ]; then
    CAOS_BUILD_COUNT=$(cat "$BUILD_DIR/build_counter.txt")
    VERSION="1.0.0+${CAOS_BUILD_COUNT}"
else
    CAOS_BUILD_COUNT="1"
    VERSION="1.0.0+1"
    echo "WARNING: Using default version"
fi

get_nodejs_paths() {
    echo "/usr/lib/x86_64-linux-gnu/nodejs"
    echo "/usr/share/nodejs"
    echo "/usr/lib/node_modules"
    echo "/usr/local/lib/node_modules"
}

get_nodejs_target_dir() {
    for path in $(get_nodejs_paths); do
        if [ -d "$path" ]; then
            echo "$path"
            return 0
        fi
    done
    echo "/usr/lib/node_modules"
}

extract_node_version() {
    local dirname=$1
    local basename=$(basename "$dirname")

    if [[ "$basename" =~ ^v([0-9]+) ]]; then
        local major_version="${BASH_REMATCH[1]}"
        if [[ "$basename" =~ ^v([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
            echo "${major_version}|${basename}"
        else
            echo "${major_version}|${basename}"
        fi
    else
        echo ""
    fi
}

extract_build_number() {
    if [ -f "$BUILD_DIR/build_counter.txt" ]; then
        cat "$BUILD_DIR/build_counter.txt"
    else
        echo "1"
    fi
}

find_node_extension_files() {
    local files=""
    if [ -d "$REPO_DIR" ]; then
        files=$(find "$REPO_DIR" -name "${PROJECT_NAME}.node" -type f 2>/dev/null)
    fi
    if [ -z "$files" ] && [ -d "$BUILD_DIR" ]; then
        files=$(find "$BUILD_DIR" -name "${PROJECT_NAME}.node" -type f 2>/dev/null)
    fi
    echo "$files"
}

check_prerequisites() {
    local ext_files=$(find_node_extension_files)
    if [ -z "$ext_files" ]; then
        echo "ERROR: No Node.js extension files found"
        echo "Expected pattern: ${PROJECT_NAME}.node"
        echo "Please build the project first:"
        echo "  cmake --build build/release --target ${PROJECT_NAME}_node_all"
        exit 1
    fi
    if ! command -v dpkg-deb >/dev/null 2>&1; then
        echo "ERROR: dpkg-deb not found. Install dpkg-dev"
        exit 1
    fi
    echo "Found $(echo "$ext_files" | wc -l) Node.js extension file(s)"
}

create_structure() {
    local deb_dir=$1
    local node_version=$2
    local node_dir_name=$3
    local TARGET_BASE=$(get_nodejs_target_dir)
    local MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"

    rm -rf "$deb_dir"
    mkdir -p "$deb_dir/$MODULE_DIR"
    mkdir -p "$deb_dir/$MODULE_DIR/types"
    mkdir -p "$deb_dir/DEBIAN"
}

copy_files() {
    local deb_dir=$1
    local source_file=$2
    local node_version=$3
    local node_dir_name=$4
    local node_napi_version=$5

    local TARGET_BASE=$(get_nodejs_target_dir)
    local MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"
    local VERSION_DIR="$deb_dir/$MODULE_DIR"

    # Copy native module
    cp "$source_file" "$VERSION_DIR/"

    # Find and copy TypeScript files from the same directory
    local source_dir=$(dirname "$source_file")
    local types_dir="$source_dir/types"
    local tsconfig_file="$source_dir/tsconfig.json"

    if [ -d "$types_dir" ]; then
        echo "  Copying TypeScript files from $types_dir"
        cp -r "$types_dir"/* "$VERSION_DIR/types/"
    else
        echo "  WARNING: TypeScript directory not found at $types_dir"
        # Create minimal TypeScript definitions
        cat > "$VERSION_DIR/types/${PROJECT_NAME}.d.ts" << 'EOF'
// TypeScript definitions for __PROJECT_NAME__
// Auto-generated - update with actual function signatures

export = CaosModule;
export as namespace caos;

interface CallContextData {
    token?: string;
}

type CaosResult<T = any> =
    | { success: true; data: T }
    | { success: false; error_type: string; error_message: string; data?: any };

interface BuildInfo {
    module: string;
    build: number;
    caos_initialized: boolean;
    node_version: string;
    napi_version: number;
    debug: boolean;
}

declare function getBuildInfo(): BuildInfo;

interface CaosModule {
    getBuildInfo: typeof getBuildInfo;
    __version__: string;
    __auth_system__: string;
    __call_context_format__: string;
    __debug__: number;
}
EOF
        # Create test file
        cat > "$VERSION_DIR/types/test-types.ts" << 'EOF'
// TypeScript test file for __PROJECT_NAME__
// Run: tsc --noEmit test-types.ts

import type * as Caos from './__PROJECT_NAME__';

// This file validates the TypeScript definitions compile correctly
console.log('TypeScript validation file for', '__PROJECT_NAME__');
EOF
    fi

    # Apply all placeholder replacements to TypeScript files
    local ts_files=$(find "$VERSION_DIR/types" -name "*.ts" -o -name "*.d.ts")
    for ts_file in $ts_files; do
        sed -i "s/__PROJECT_NAME__/${PROJECT_NAME}/g" "$ts_file"
        sed -i "s/__DB_BACKEND__/${DB_BACKEND}/g" "$ts_file"
        sed -i "s/__CAOS_BUILD_COUNT__/${CAOS_BUILD_COUNT}/g" "$ts_file"
        sed -i "s/__TIMESTAMP__/$(date)/g" "$ts_file"
        sed -i "s/__NODE_MAJOR_VER__/${node_version}/g" "$ts_file"
        sed -i "s/__NODE_NAPI_VERSION__/${node_napi_version}/g" "$ts_file"
    done

    if [ -f "$tsconfig_file" ]; then
        cp "$tsconfig_file" "$VERSION_DIR/"
    else
        # Create minimal tsconfig.json
        cat > "$VERSION_DIR/tsconfig.json" << EOF
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "CommonJS",
    "declaration": true,
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true
  },
  "include": ["types/**/*.ts", "types/**/*.d.ts"],
  "exclude": ["node_modules", "dist", "build"]
}
EOF
    fi

    # Create launcher.js with TypeScript hint
    cat > "$VERSION_DIR/launcher.js" << 'EOF'
#!/usr/bin/env node
/**
 * __PROJECT_NAME__ - CAOSDBA Node.js Native Bindings
 *
 * @module __PROJECT_NAME__
 * @see ./types/__PROJECT_NAME__.d.ts for TypeScript definitions
 */
const path = require('path');
const fs = require('fs');
const PROJECT_NAME = '__PROJECT_NAME__';

try {
    const modulePath = path.join(__dirname, '__PROJECT_NAME__.node');
    if (fs.existsSync(modulePath)) {
        const native = require(modulePath);

        // Add TypeScript hint to the module
        if (native.getBuildInfo) {
            Object.defineProperty(native, '__typescript_hint__', {
                value: 'TypeScript definitions available in ./types/',
                enumerable: false,
                writable: false
            });
        }

        if (require.main === module && native.getBuildInfo) {
            console.log(JSON.stringify(native.getBuildInfo(), null, 2));
        }
        module.exports = native;
    } else {
        throw new Error(`Native module not found: ${modulePath}`);
    }
} catch (error) {
    console.error(`Error loading ${PROJECT_NAME}:`, error.message);
    process.exit(1);
}
EOF

    sed -i "s/__PROJECT_NAME__/${PROJECT_NAME}/g" "$VERSION_DIR/launcher.js"

    # Create package.json with types field
    cat > "$VERSION_DIR/package.json" << EOF
{
  "name": "${PROJECT_NAME}-${node_dir_name}",
  "version": "0.1.0",
  "main": "./launcher.js",
  "types": "./types/${PROJECT_NAME}.d.ts",
  "description": "CAOS Native Node.js Bindings for ${DB_BACKEND} (Node.js ${node_dir_name})",
  "engines": {
    "node": ">=${node_version}.0.0"
  }
}
EOF

    ln -sf "launcher.js" "$VERSION_DIR/index.js"

    # Set permissions for TypeScript files
    chmod 644 "$VERSION_DIR"/types/* 2>/dev/null || true
    chmod 755 "$VERSION_DIR"/launcher.js 2>/dev/null || true
}

create_version_postinst() {
    local deb_dir=$1
    local node_version=$2
    local node_dir_name=$3

    local TARGET_BASE=$(get_nodejs_target_dir)
    local VERSION_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"
    local MAIN_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}"

    cat > "$deb_dir/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

VERSION_MODULE_DIR="__VERSION_MODULE_DIR__"
MAIN_MODULE_DIR="__MAIN_MODULE_DIR__"
PROJECT_NAME="__PROJECT_NAME__"
NODE_DIR_NAME="__NODE_DIR_NAME__"

echo "Configuring ${PROJECT_NAME} Node.js module for version ${NODE_DIR_NAME}..."

if [ -d "$VERSION_MODULE_DIR" ]; then
    chmod 755 "$VERSION_MODULE_DIR" 2>/dev/null || true
    chmod 644 "$VERSION_MODULE_DIR"/*.node 2>/dev/null || true
    chmod 644 "$VERSION_MODULE_DIR"/*.json 2>/dev/null || true
    chmod 755 "$VERSION_MODULE_DIR"/launcher.js 2>/dev/null || true

    # Set permissions for TypeScript files
    if [ -d "$VERSION_MODULE_DIR/types" ]; then
        chmod 755 "$VERSION_MODULE_DIR/types" 2>/dev/null || true
        chmod 644 "$VERSION_MODULE_DIR"/types/* 2>/dev/null || true
    fi

    # ALWAYS create/update main directory and launcher.js
    mkdir -p "$MAIN_MODULE_DIR"

    # Create main types directory if it doesn't exist
    mkdir -p "$MAIN_MODULE_DIR/types"

    # Copy TypeScript files from this version to main directory (if main doesn't have them)
    if [ -d "$VERSION_MODULE_DIR/types" ] && [ ! -f "$MAIN_MODULE_DIR/types/${PROJECT_NAME}.d.ts" ]; then
        cp -r "$VERSION_MODULE_DIR"/types/* "$MAIN_MODULE_DIR/types/"
        echo "  Copied TypeScript definitions to main directory"
    fi

    # Create main launcher.js - ALWAYS overwrite to ensure it exists
    cat > "$MAIN_MODULE_DIR/launcher.js" << 'LAUNCHER_EOF'
#!/usr/bin/env node
// ${PROJECT_NAME} - Main auto-version loader
// TypeScript definitions available in ./types/
const path = require('path');
const fs = require('fs');
const PROJECT_NAME = '__PROJECT_NAME__';

function loadNativeModule() {
    const moduleDir = __dirname;
    const nodeVersion = process.versions.node;
    const nodeMajor = nodeVersion.split('.')[0];

    // Try exact version first (e.g., v18.19.1 -> v18.19.1)
    const exactVersion = `v${nodeVersion}`;
    const exactModule = path.join(moduleDir, exactVersion, `${PROJECT_NAME}.node`);
    if (fs.existsSync(exactModule)) {
        return require(path.join(moduleDir, exactVersion, 'launcher.js'));
    }

    // Try major version (e.g., v18)
    const majorVersion = `v${nodeMajor}`;
    const majorModule = path.join(moduleDir, majorVersion, `${PROJECT_NAME}.node`);
    if (fs.existsSync(majorModule)) {
        return require(path.join(moduleDir, majorVersion, 'launcher.js'));
    }

    // Find any compatible version
    if (fs.existsSync(moduleDir)) {
        const files = fs.readdirSync(moduleDir);
        const availableVersions = [];

        for (const file of files) {
            if (file.startsWith('v') && fs.statSync(path.join(moduleDir, file)).isDirectory()) {
                const versionModule = path.join(moduleDir, file, `${PROJECT_NAME}.node`);
                if (fs.existsSync(versionModule)) {
                    availableVersions.push(file);
                }
            }
        }

        if (availableVersions.length > 0) {
            availableVersions.sort((a, b) => {
                const aVer = parseInt(a.replace('v', '').split('.')[0]);
                const bVer = parseInt(b.replace('v', '').split('.')[0]);
                return Math.abs(aVer - nodeMajor) - Math.abs(bVer - nodeMajor);
            });

            const closestVersion = availableVersions[0];
            console.warn(`[${PROJECT_NAME}] Using ${closestVersion} module for Node.js ${nodeVersion}`);
            return require(path.join(moduleDir, closestVersion, 'launcher.js'));
        }
    }

    throw new Error(`No ${PROJECT_NAME} module found for Node.js ${nodeVersion}. Available versions: ${availableVersions ? availableVersions.join(', ') : 'none'}`);
}

try {
    const native = loadNativeModule();

    if (require.main === module) {
        if (native.getBuildInfo) {
            console.log(JSON.stringify(native.getBuildInfo(), null, 2));
        } else {
            console.log(`{ "module": "${PROJECT_NAME}", "status": "loaded" }`);
        }
    }

    module.exports = native;

} catch (error) {
    console.error(`Error loading ${PROJECT_NAME}: ${error.message}`);
    console.error('');
    console.error('Try:');
    console.error('  1. Set NODE_PATH:');
    console.error(`     NODE_PATH=__TARGET_BASE__ node -e "const m = require('${PROJECT_NAME}'); console.log(m.getBuildInfo())"`);
    console.error('  2. Use absolute path:');
    console.error(`     node -e "const m = require('__MAIN_MODULE_DIR__/launcher.js'); console.log(m.getBuildInfo())"`);
    console.error('  3. Check TypeScript definitions:');
    console.error(`     ls -la __MAIN_MODULE_DIR__/types/`);
    console.error('  4. Reinstall:');
    console.error(`     sudo apt reinstall __PROJECT_NAME_SANITIZED__-nodejs-__DB_BACKEND_LOWER__`);
    console.error('');
    process.exit(1);
}
LAUNCHER_EOF

    # Replace placeholders in launcher.js
    sed -i "s/__PROJECT_NAME__/$PROJECT_NAME/g" "$MAIN_MODULE_DIR/launcher.js"
    sed -i "s/__PROJECT_NAME_SANITIZED__/$PROJECT_NAME_SANITIZED/g" "$MAIN_MODULE_DIR/launcher.js"
    sed -i "s/__DB_BACKEND_LOWER__/$DB_BACKEND_LOWER/g" "$MAIN_MODULE_DIR/launcher.js"
    sed -i "s|__TARGET_BASE__|$TARGET_BASE|g" "$MAIN_MODULE_DIR/launcher.js"
    sed -i "s|__MAIN_MODULE_DIR__|$MAIN_MODULE_DIR|g" "$MAIN_MODULE_DIR/launcher.js"

    chmod 755 "$MAIN_MODULE_DIR/launcher.js"

    # Create main package.json with types field
    cat > "$MAIN_MODULE_DIR/package.json" << PKG_EOF
{
  "name": "$PROJECT_NAME",
  "version": "0.1.0",
  "main": "./launcher.js",
  "types": "./types/${PROJECT_NAME}.d.ts",
  "description": "CAOS Native Node.js Bindings for $DB_BACKEND",
  "engines": {
    "node": ">=14.0.0"
  }
}
PKG_EOF

    # Copy tsconfig.json to main directory if it exists in version directory
    if [ -f "$VERSION_MODULE_DIR/tsconfig.json" ] && [ ! -f "$MAIN_MODULE_DIR/tsconfig.json" ]; then
        cp "$VERSION_MODULE_DIR/tsconfig.json" "$MAIN_MODULE_DIR/"
    fi

    ln -sf "launcher.js" "$MAIN_MODULE_DIR/index.js"

    # Create symlink in /usr/local/lib/node_modules for global access
    if [ "$TARGET_BASE" != "/usr/local/lib/node_modules" ] && [ -d "/usr/local/lib/node_modules" ]; then
        if [ ! -L "/usr/local/lib/node_modules/$PROJECT_NAME" ] || [ ! -d "/usr/local/lib/node_modules/$PROJECT_NAME" ]; then
            ln -sf "$MAIN_MODULE_DIR" "/usr/local/lib/node_modules/$PROJECT_NAME" 2>/dev/null || true
            echo "  Created symlink in /usr/local/lib/node_modules/"
        fi
    fi

    echo "  Installed Node.js module for version: $NODE_DIR_NAME"
    echo "  TypeScript definitions: $MAIN_MODULE_DIR/types/${PROJECT_NAME}.d.ts"

else
    echo "  ERROR: Module directory not found: $VERSION_MODULE_DIR"
    exit 1
fi

echo "Installation completed."
EOF

    sed -i "s|__VERSION_MODULE_DIR__|$VERSION_MODULE_DIR|g" "$deb_dir/DEBIAN/postinst"
    sed -i "s|__MAIN_MODULE_DIR__|$MAIN_MODULE_DIR|g" "$deb_dir/DEBIAN/postinst"
    sed -i "s/__PROJECT_NAME__/$PROJECT_NAME/g" "$deb_dir/DEBIAN/postinst"
    sed -i "s/__NODE_DIR_NAME__/$node_dir_name/g" "$deb_dir/DEBIAN/postinst"
    sed -i "s/__PROJECT_NAME_SANITIZED__/$PROJECT_NAME_SANITIZED/g" "$deb_dir/DEBIAN/postinst"
    sed -i "s/__DB_BACKEND_LOWER__/$DB_BACKEND_LOWER/g" "$deb_dir/DEBIAN/postinst"
    sed -i "s|__TARGET_BASE__|$TARGET_BASE|g" "$deb_dir/DEBIAN/postinst"

    chmod +x "$deb_dir/DEBIAN/postinst"
}

create_version_prerm() {
    local deb_dir=$1
    local node_version=$2
    local node_dir_name=$3

    local TARGET_BASE=$(get_nodejs_target_dir)
    local VERSION_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"
    local MAIN_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}"

    cat > "$deb_dir/DEBIAN/prerm" << 'EOF'
#!/bin/bash
set -e

VERSION_MODULE_DIR="__VERSION_MODULE_DIR__"
MAIN_MODULE_DIR="__MAIN_MODULE_DIR__"
PROJECT_NAME="__PROJECT_NAME__"

case "$1" in
    remove|purge)
        echo "Removing ${PROJECT_NAME} module for version __NODE_DIR_NAME__..."

        if [ -d "$VERSION_MODULE_DIR" ]; then
            rm -rf "$VERSION_MODULE_DIR"
            echo "Removed version directory: $VERSION_MODULE_DIR"
        fi

        # Only remove main directory if no versions left
        if [ -d "$MAIN_MODULE_DIR" ]; then
            remaining_versions=0
            for dir in "$MAIN_MODULE_DIR"/v*; do
                [ -d "$dir" ] || continue

                if [ -d "$dir" ]; then
                    remaining_versions=$((remaining_versions + 1))
                fi
            done

            if [ "$remaining_versions" -eq 0 ]; then
                rm -rf "$MAIN_MODULE_DIR"
                echo "Removed main directory (no versions left)"

                # Remove symlink if it exists
                if [ -L "/usr/local/lib/node_modules/$PROJECT_NAME" ]; then
                    rm -f "/usr/local/lib/node_modules/$PROJECT_NAME"
                    echo "Removed symlink from /usr/local/lib/node_modules/"
                fi
            fi
        fi
        ;;
esac
EOF

    sed -i "s|__VERSION_MODULE_DIR__|$VERSION_MODULE_DIR|g" "$deb_dir/DEBIAN/prerm"
    sed -i "s|__MAIN_MODULE_DIR__|$MAIN_MODULE_DIR|g" "$deb_dir/DEBIAN/prerm"
    sed -i "s/__PROJECT_NAME__/$PROJECT_NAME/g" "$deb_dir/DEBIAN/prerm"
    sed -i "s/__NODE_DIR_NAME__/$node_dir_name/g" "$deb_dir/DEBIAN/prerm"

    chmod +x "$deb_dir/DEBIAN/prerm"
}

create_version_control() {
    local deb_dir=$1
    local package_name=$2
    local node_version=$3
    local node_dir_name=$4

    local conflicts_clause=""
    case "${DB_BACKEND_LOWER}" in
        "mysql")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-nodejs-mariadb-${node_dir_name}, ${PROJECT_NAME_SANITIZED}-nodejs-postgresql-${node_dir_name}"
            ;;
        "mariadb")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-nodejs-mysql-${node_dir_name}, ${PROJECT_NAME_SANITIZED}-nodejs-postgresql-${node_dir_name}"
            ;;
        "postgresql")
            conflicts_clause="${PROJECT_NAME_SANITIZED}-nodejs-mysql-${node_dir_name}, ${PROJECT_NAME_SANITIZED}-nodejs-mariadb-${node_dir_name}"
            ;;
    esac

    cat > "$deb_dir/DEBIAN/control" << EOF
Package: $package_name
Version: $VERSION
Architecture: amd64
Maintainer: CAOSDBA Development Team
Description: ${PROJECT_NAME} - CAOSDBA extension for Node.js ${node_dir_name} with $DB_BACKEND_LOWER backend
 Native Node.js bindings for CAOSDBA database operations.
 Includes TypeScript definitions for type-safe development.
 Installs to: $(get_nodejs_target_dir)/${PROJECT_NAME}/${node_dir_name}/
Depends: nodejs (>= ${node_version}.0.0), ${PACKAGE_NAME_BASE}
Conflicts: $conflicts_clause
Section: javascript
Priority: optional
EOF
}

build_single_package() {
    local source_file=$1
    local node_version=$2
    local node_dir_name=$3
    local build_number=$4

    # Try to get N-API version using node binary.
    # Attempting node major binary (e.g. node18) first, then default node.
    local node_napi_version=$(node${node_version} -p "process.versions.napi" 2>/dev/null || node -p "process.versions.napi" 2>/dev/null || echo "unknown")

    local safe_dir_name=$(echo "$node_dir_name" | tr '.' '_' | tr '-' '_')
    local package_name="${PACKAGE_NAME_BASE}-${safe_dir_name}"

    local deb_dir="$BUILD_DIR/deb-nodejs-${safe_dir_name}"

    rm -rf "$deb_dir"
    create_structure "$deb_dir" "$node_version" "$node_dir_name"
    copy_files "$deb_dir" "$source_file" "$node_version" "$node_dir_name" "$node_napi_version"

    create_version_postinst "$deb_dir" "$node_version" "$node_dir_name"
    create_version_prerm "$deb_dir" "$node_version" "$node_dir_name"
    create_version_control "$deb_dir" "$package_name" "$node_version" "$node_dir_name"

    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${package_name}_${VERSION}_amd64.deb" >/dev/null 2>&1

    rm -rf "$deb_dir"
    echo "Built: ${package_name}_${VERSION}_amd64.deb (Node.js ${node_dir_name})"
    echo "  Installs to: $(get_nodejs_target_dir)/${PROJECT_NAME}/${node_dir_name}/"
    echo "  TypeScript definitions included"
    return 0
}

create_meta_package() {
    local deb_dir="$BUILD_DIR/deb-meta-node"
    rm -rf "$deb_dir"
    mkdir -p "$deb_dir/DEBIAN"

    # Collect available major versions
    local all_versions=""
    shopt -s nullglob
    for pkg_file in "$BUILD_DIR"/*-nodejs-${DB_BACKEND_LOWER}-v*.deb; do
        if [[ "$(basename "$pkg_file")" =~ ${PACKAGE_NAME_BASE}-v([0-9]+)_ ]]; then
            local version="${BASH_REMATCH[1]}"
            if [ -n "$all_versions" ]; then
                all_versions="$all_versions, "
            fi
            all_versions="$all_versions$version"
        fi
    done
    shopt -u nullglob

    cat > "$deb_dir/DEBIAN/control" << EOF
Package: ${PACKAGE_NAME_BASE}
Version: $VERSION
Architecture: all
Maintainer: CAOSDBA Development Team
Description: ${PROJECT_NAME} - CAOSDBA Node.js extension with $DB_BACKEND_LOWER backend (metapackage)
 This metapackage installs the appropriate Node.js version module.
 Includes TypeScript definitions for type-safe development.
 Available for Node.js versions: ${all_versions}
Section: javascript
Priority: optional
Recommends: ${PACKAGE_NAME_BASE}-v18 | \
            ${PACKAGE_NAME_BASE}-v20 | \
            ${PACKAGE_NAME_BASE}-v22
Conflicts: ${PROJECT_NAME_SANITIZED}-nodejs-mysql, ${PROJECT_NAME_SANITIZED}-nodejs-mariadb, ${PROJECT_NAME_SANITIZED}-nodejs-postgresql
EOF

    cat > "$deb_dir/DEBIAN/postinst" << 'EOF'
#!/bin/bash
set -e

echo "=========================================="
echo "${PROJECT_NAME} Node.js Module Installed"
echo "=========================================="
echo ""
echo "To use ${PROJECT_NAME} in your Node.js application:"
echo ""
echo "Method 1: Set NODE_PATH (recommended):"
echo "  NODE_PATH=/usr/lib/x86_64-linux-gnu/nodejs node -e \"const m = require('${PROJECT_NAME}'); console.log(m.getBuildInfo())\""
echo ""
echo "Method 2: Use absolute path:"
echo "  node -e \"const m = require('/usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/launcher.js'); console.log(m.getBuildInfo())\""
echo ""
echo "TypeScript Development:"
echo "  TypeScript definitions are included in:"
echo "  /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/types/${PROJECT_NAME}.d.ts"
echo ""
echo "  To validate types:"
echo "  cd /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/"
echo "  tsc --noEmit --project tsconfig.json"
echo ""
echo "=========================================="
EOF

    sed -i "s/\${PROJECT_NAME}/${PROJECT_NAME}/g" "$deb_dir/DEBIAN/postinst"
    chmod +x "$deb_dir/DEBIAN/postinst"

    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${PACKAGE_NAME_BASE}_${VERSION}_all.deb" >/dev/null 2>&1
    rm -rf "$deb_dir"

    echo "Built meta-package: ${PACKAGE_NAME_BASE}_${VERSION}_all.deb"
    echo "  Includes TypeScript definitions"
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
            echo "\$package_name optional nodejs" >> "\$OVERRIDE_FILE"
        fi
    fi
done

cd "\$REPO_DIR" || exit 1
rm -f Release.gpg InRelease

dpkg-scanpackages --multiversion . "\$OVERRIDE_FILE" > Packages
gzip -k -f Packages

cat > Release << RELEASE_CONTENT
Origin: ${PROJECT_NAME} Node.js Repository
Label: ${PROJECT_NAME}
Suite: stable
Codename: ${PROJECT_NAME}
Version: 1.0
Architectures: amd64 all
Components: main
Description: ${PROJECT_NAME} - CAOSDBA Node.js extension with TypeScript support
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
if [ "\$EUID" -ne 0 ]; then
    echo "ERROR: Run as root"
    exit 1
fi

REPO_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
if [ ! -f "\$REPO_DIR/Packages.gz" ]; then
    echo "ERROR: Run ./update-repo.sh first"
    exit 1
fi

echo "deb [trusted=yes] file:\$REPO_DIR ./" > /etc/apt/sources.list.d/${PROJECT_NAME}-nodejs-local.list
apt-get update -oAcquire::AllowInsecureRepositories=true

echo ""
echo "=== ${PROJECT_NAME} Repository Ready ==="
echo ""
echo "Install the meta-package:"
echo "  sudo apt install ${PACKAGE_NAME_BASE}"
echo ""
echo "TypeScript definitions are included in the package."
echo "Usage instructions in /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/launcher.js"
EOF
    chmod +x "$REPO_DIR/setup-apt-source.sh"
}

move_packages_to_repository() {
    mkdir -p "$REPO_DIR"
    local moved_count=0
    for pkg_file in "$BUILD_DIR"/*.deb; do
        if [ -f "$pkg_file" ]; then
            mv "$pkg_file" "$REPO_DIR/"
            moved_count=$((moved_count + 1))
        fi
    done
    echo "Moved $moved_count packages to $REPO_DIR"
}

main() {
    check_prerequisites
    local ext_files=$(find_node_extension_files)
    local built_count=0

    while IFS= read -r source_file; do
        [ -z "$source_file" ] && continue

        local source_dir=$(dirname "$source_file")
        local dir_name=$(basename "$source_dir")
        local version_info=$(extract_node_version "$dir_name")

        if [ -z "$version_info" ]; then
            local parent_dir=$(dirname "$source_dir")
            local parent_name=$(basename "$parent_dir")
            version_info=$(extract_node_version "$parent_name")
        fi

        if [ -z "$version_info" ]; then
            echo "WARNING: Cannot extract version from: $source_file"
            continue
        fi

        IFS='|' read -r node_major node_dir_name <<< "$version_info"
        if [ -z "$node_major" ] || [ -z "$node_dir_name" ]; then
            echo "WARNING: Invalid version format: $version_info"
            continue
        fi

        if build_single_package "$source_file" "$node_major" "$node_dir_name" "$CAOS_BUILD_COUNT"; then
            built_count=$((built_count + 1))
        fi
    done <<< "$ext_files"

    if [ "$built_count" -gt 0 ]; then
        create_meta_package
        move_packages_to_repository
        create_repository_scripts

        echo "SUCCESS: Built $built_count Node.js packages"
        echo "Repository: $REPO_DIR"
        echo "Test installation:"
        echo "  sudo apt install ${PACKAGE_NAME_BASE}"
        echo ""
        echo "After installation, test with:"
        echo "  NODE_PATH=/usr/lib/x86_64-linux-gnu/nodejs node -e \"const m = require('${PROJECT_NAME}'); console.log(m.getBuildInfo())\""
        echo ""
        echo "TypeScript development:"
        echo "  TypeScript definitions installed to:"
        echo "  /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/types/${PROJECT_NAME}.d.ts"
        return 0
    else
        echo "ERROR: No packages built"
        return 1
    fi
}

main "$@"

##!/bin/bash

#DB_BACKEND=${1:-"MYSQL"}
#PROJECT_NAME=${2:-"caos"}
#PROJECT_NAME_SANITIZED=$(echo "$PROJECT_NAME" | tr '_' '-')
#DB_BACKEND_LOWER=$(echo "$DB_BACKEND" | tr '[:upper:]' '[:lower:]')

#SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
#PROJECT_ROOT="$(cd "$SCRIPT_DIR/../" && pwd)"
#BUILD_DIR="$PROJECT_ROOT/build/release"
#DIST_DIR="$PROJECT_ROOT/dist"
#REPO_DIR="$DIST_DIR/repositories/${PROJECT_NAME}/nodejs"
#PACKAGE_NAME_BASE="${PROJECT_NAME_SANITIZED}-nodejs-${DB_BACKEND_LOWER}"

#if [ -f "$BUILD_DIR/build_counter.txt" ]; then
#    CAOS_BUILD_COUNTER=$(cat "$BUILD_DIR/build_counter.txt")
#    VERSION="1.0.0+${CAOS_BUILD_COUNTER}"
#else
#    VERSION="1.0.0+1"
#    echo "WARNING: Using default version"
#fi

#get_nodejs_paths() {
#    echo "/usr/lib/x86_64-linux-gnu/nodejs"
#    echo "/usr/share/nodejs"
#    echo "/usr/lib/node_modules"
#    echo "/usr/local/lib/node_modules"
#}

#get_nodejs_target_dir() {
#    for path in $(get_nodejs_paths); do
#        if [ -d "$path" ]; then
#            echo "$path"
#            return 0
#        fi
#    done
#    echo "/usr/lib/node_modules"
#}

#extract_node_version() {
#    local dirname=$1
#    local basename=$(basename "$dirname")

#    if [[ "$basename" =~ ^v([0-9]+) ]]; then
#        local major_version="${BASH_REMATCH[1]}"
#        if [[ "$basename" =~ ^v([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
#            echo "${major_version}|${basename}"
#        else
#            echo "${major_version}|${basename}"
#        fi
#    else
#        echo ""
#    fi
#}

#extract_build_number() {
#    if [ -f "$BUILD_DIR/build_counter.txt" ]; then
#        cat "$BUILD_DIR/build_counter.txt"
#    else
#        echo "1"
#    fi
#}

#find_node_extension_files() {
#    local files=""
#    if [ -d "$REPO_DIR" ]; then
#        files=$(find "$REPO_DIR" -name "${PROJECT_NAME}.node" -type f 2>/dev/null)
#    fi
#    if [ -z "$files" ] && [ -d "$BUILD_DIR" ]; then
#        files=$(find "$BUILD_DIR" -name "${PROJECT_NAME}.node" -type f 2>/dev/null)
#    fi
#    echo "$files"
#}

#check_prerequisites() {
#    local ext_files=$(find_node_extension_files)
#    if [ -z "$ext_files" ]; then
#        echo "ERROR: No Node.js extension files found"
#        echo "Expected pattern: ${PROJECT_NAME}.node"
#        echo "Please build the project first:"
#        echo "  cmake --build build/release --target ${PROJECT_NAME}_node_all"
#        exit 1
#    fi
#    if ! command -v dpkg-deb >/dev/null 2>&1; then
#        echo "ERROR: dpkg-deb not found. Install dpkg-dev"
#        exit 1
#    fi
#    echo "Found $(echo "$ext_files" | wc -l) Node.js extension file(s)"
#}

#create_structure() {
#    local deb_dir=$1
#    local node_version=$2
#    local node_dir_name=$3
#    local TARGET_BASE=$(get_nodejs_target_dir)
#    local MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"

#    rm -rf "$deb_dir"
#    mkdir -p "$deb_dir/$MODULE_DIR"
#    mkdir -p "$deb_dir/$MODULE_DIR/types"
#    mkdir -p "$deb_dir/DEBIAN"
#}

#copy_files() {
#    local deb_dir=$1
#    local source_file=$2
#    local node_version=$3
#    local node_dir_name=$4

#    local TARGET_BASE=$(get_nodejs_target_dir)
#    local MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"
#    local VERSION_DIR="$deb_dir/$MODULE_DIR"

#    # Copy native module
#    cp "$source_file" "$VERSION_DIR/"

#    # Find and copy TypeScript files from the same directory
#    local source_dir=$(dirname "$source_file")
#    local types_dir="$source_dir/types"
#    local tsconfig_file="$source_dir/tsconfig.json"

#    if [ -d "$types_dir" ]; then
#        echo "  Copying TypeScript files from $types_dir"
#        cp -r "$types_dir"/* "$VERSION_DIR/types/"
#    else
#        echo "  WARNING: TypeScript directory not found at $types_dir"
#        # Create minimal TypeScript definitions
#        cat > "$VERSION_DIR/types/${PROJECT_NAME}.d.ts" << 'EOF'
#// TypeScript definitions for __PROJECT_NAME__
#// Auto-generated - update with actual function signatures

#export = CaosModule;
#export as namespace caos;

#interface CallContextData {
#    token?: string;
#}

#type CaosResult<T = any> =
#    | { success: true; data: T }
#    | { success: false; error_type: string; error_message: string; data?: any };

#interface BuildInfo {
#    module: string;
#    build: number;
#    caos_initialized: boolean;
#    node_version: string;
#    napi_version: number;
#    debug: boolean;
#}

#declare function getBuildInfo(): BuildInfo;

#// TODO: Add actual function declarations from bindings.cpp
#// Example:
#// declare function IQuery_Template_echoString(
#//     callContext: CallContextData,
#//     input: string
#// ): CaosResult<string>;

#interface CaosModule {
#    getBuildInfo: typeof getBuildInfo;
#    __version__: string;
#    __auth_system__: string;
#    __call_context_format__: string;
#    __debug__: number;
#}
#EOF
#        sed -i "s/__PROJECT_NAME__/${PROJECT_NAME}/g" "$VERSION_DIR/types/${PROJECT_NAME}.d.ts"

#        # Create test file
#        cat > "$VERSION_DIR/types/test-types.ts" << 'EOF'
#// TypeScript test file for __PROJECT_NAME__
#// Run: tsc --noEmit test-types.ts

#import type * as Caos from './__PROJECT_NAME__';

#// This file validates the TypeScript definitions compile correctly
#console.log('TypeScript validation file for', '__PROJECT_NAME__');
#EOF
#        sed -i "s/__PROJECT_NAME__/${PROJECT_NAME}/g" "$VERSION_DIR/types/test-types.ts"
#    fi

#    if [ -f "$tsconfig_file" ]; then
#        cp "$tsconfig_file" "$VERSION_DIR/"
#    else
#        # Create minimal tsconfig.json
#        cat > "$VERSION_DIR/tsconfig.json" << EOF
#{
#  "compilerOptions": {
#    "target": "ES2020",
#    "module": "CommonJS",
#    "declaration": true,
#    "strict": true,
#    "esModuleInterop": true,
#    "skipLibCheck": true,
#    "forceConsistentCasingInFileNames": true
#  },
#  "include": ["types/**/*.ts", "types/**/*.d.ts"],
#  "exclude": ["node_modules", "dist", "build"]
#}
#EOF
#    fi

#    # Create launcher.js with TypeScript hint
#    cat > "$VERSION_DIR/launcher.js" << 'EOF'
##!/usr/bin/env node
#/**
# * __PROJECT_NAME__ - CAOSDBA Node.js Native Bindings
# *
# * @module __PROJECT_NAME__
# * @see ./types/__PROJECT_NAME__.d.ts for TypeScript definitions
# */
#const path = require('path');
#const fs = require('fs');
#const PROJECT_NAME = '__PROJECT_NAME__';

#try {
#    const modulePath = path.join(__dirname, '__PROJECT_NAME__.node');
#    if (fs.existsSync(modulePath)) {
#        const native = require(modulePath);

#        // Add TypeScript hint to the module
#        if (native.getBuildInfo) {
#            Object.defineProperty(native, '__typescript_hint__', {
#                value: 'TypeScript definitions available in ./types/',
#                enumerable: false,
#                writable: false
#            });
#        }

#        if (require.main === module && native.getBuildInfo) {
#            console.log(JSON.stringify(native.getBuildInfo(), null, 2));
#        }
#        module.exports = native;
#    } else {
#        throw new Error(`Native module not found: ${modulePath}`);
#    }
#} catch (error) {
#    console.error(`Error loading ${PROJECT_NAME}:`, error.message);
#    process.exit(1);
#}
#EOF

#    sed -i "s/__PROJECT_NAME__/${PROJECT_NAME}/g" "$VERSION_DIR/launcher.js"

#    # Create package.json with types field
#    cat > "$VERSION_DIR/package.json" << EOF
#{
#  "name": "${PROJECT_NAME}-${node_dir_name}",
#  "version": "0.1.0",
#  "main": "./launcher.js",
#  "types": "./types/${PROJECT_NAME}.d.ts",
#  "description": "CAOS Native Node.js Bindings for ${DB_BACKEND} (Node.js ${node_dir_name})",
#  "engines": {
#    "node": ">=${node_version}.0.0"
#  }
#}
#EOF

#    ln -s "launcher.js" "$VERSION_DIR/index.js"

#    # Set permissions for TypeScript files
#    chmod 644 "$VERSION_DIR"/types/* 2>/dev/null || true
#    chmod 755 "$VERSION_DIR"/launcher.js 2>/dev/null || true
#}

#create_version_postinst() {
#    local deb_dir=$1
#    local node_version=$2
#    local node_dir_name=$3

#    local TARGET_BASE=$(get_nodejs_target_dir)
#    local VERSION_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"
#    local MAIN_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}"

#    cat > "$deb_dir/DEBIAN/postinst" << 'EOF'
##!/bin/bash
#set -e

#VERSION_MODULE_DIR="__VERSION_MODULE_DIR__"
#MAIN_MODULE_DIR="__MAIN_MODULE_DIR__"
#PROJECT_NAME="__PROJECT_NAME__"
#NODE_DIR_NAME="__NODE_DIR_NAME__"

#echo "Configuring ${PROJECT_NAME} Node.js module for version ${NODE_DIR_NAME}..."

#if [ -d "$VERSION_MODULE_DIR" ]; then
#    chmod 755 "$VERSION_MODULE_DIR" 2>/dev/null || true
#    chmod 644 "$VERSION_MODULE_DIR"/*.node 2>/dev/null || true
#    chmod 644 "$VERSION_MODULE_DIR"/*.json 2>/dev/null || true
#    chmod 755 "$VERSION_MODULE_DIR"/launcher.js 2>/dev/null || true

#    # Set permissions for TypeScript files
#    if [ -d "$VERSION_MODULE_DIR/types" ]; then
#        chmod 755 "$VERSION_MODULE_DIR/types" 2>/dev/null || true
#        chmod 644 "$VERSION_MODULE_DIR"/types/* 2>/dev/null || true
#    fi

#    # ALWAYS create/update main directory and launcher.js
#    mkdir -p "$MAIN_MODULE_DIR"

#    # Create main types directory if it doesn't exist
#    mkdir -p "$MAIN_MODULE_DIR/types"

#    # Copy TypeScript files from this version to main directory (if main doesn't have them)
#    if [ -d "$VERSION_MODULE_DIR/types" ] && [ ! -f "$MAIN_MODULE_DIR/types/${PROJECT_NAME}.d.ts" ]; then
#        cp -r "$VERSION_MODULE_DIR"/types/* "$MAIN_MODULE_DIR/types/"
#        echo "  Copied TypeScript definitions to main directory"
#    fi

#    # Create main launcher.js - ALWAYS overwrite to ensure it exists
#    cat > "$MAIN_MODULE_DIR/launcher.js" << 'LAUNCHER_EOF'
##!/usr/bin/env node
#// ${PROJECT_NAME} - Main auto-version loader
#// TypeScript definitions available in ./types/
#const path = require('path');
#const fs = require('fs');
#const PROJECT_NAME = '__PROJECT_NAME__';

#function loadNativeModule() {
#    const moduleDir = __dirname;
#    const nodeVersion = process.versions.node;
#    const nodeMajor = nodeVersion.split('.')[0];

#    // Try exact version first (e.g., v18.19.1 -> v18.19.1)
#    const exactVersion = `v${nodeVersion}`;
#    const exactModule = path.join(moduleDir, exactVersion, `${PROJECT_NAME}.node`);
#    if (fs.existsSync(exactModule)) {
#        return require(path.join(moduleDir, exactVersion, 'launcher.js'));
#    }

#    // Try major version (e.g., v18)
#    const majorVersion = `v${nodeMajor}`;
#    const majorModule = path.join(moduleDir, majorVersion, `${PROJECT_NAME}.node`);
#    if (fs.existsSync(majorModule)) {
#        return require(path.join(moduleDir, majorVersion, 'launcher.js'));
#    }

#    // Find any compatible version
#    if (fs.existsSync(moduleDir)) {
#        const files = fs.readdirSync(moduleDir);
#        const availableVersions = [];

#        for (const file of files) {
#            if (file.startsWith('v') && fs.statSync(path.join(moduleDir, file)).isDirectory()) {
#                const versionModule = path.join(moduleDir, file, `${PROJECT_NAME}.node`);
#                if (fs.existsSync(versionModule)) {
#                    availableVersions.push(file);
#                }
#            }
#        }

#        if (availableVersions.length > 0) {
#            availableVersions.sort((a, b) => {
#                const aVer = parseInt(a.replace('v', '').split('.')[0]);
#                const bVer = parseInt(b.replace('v', '').split('.')[0]);
#                return Math.abs(aVer - nodeMajor) - Math.abs(bVer - nodeMajor);
#            });

#            const closestVersion = availableVersions[0];
#            console.warn(`[${PROJECT_NAME}] Using ${closestVersion} module for Node.js ${nodeVersion}`);
#            return require(path.join(moduleDir, closestVersion, 'launcher.js'));
#        }
#    }

#    throw new Error(`No ${PROJECT_NAME} module found for Node.js ${nodeVersion}. Available versions: ${availableVersions ? availableVersions.join(', ') : 'none'}`);
#}

#try {
#    const native = loadNativeModule();

#    if (require.main === module) {
#        if (native.getBuildInfo) {
#            console.log(JSON.stringify(native.getBuildInfo(), null, 2));
#        } else {
#            console.log(`{ "module": "${PROJECT_NAME}", "status": "loaded" }`);
#        }
#    }

#    module.exports = native;

#} catch (error) {
#    console.error(`Error loading ${PROJECT_NAME}: ${error.message}`);
#    console.error('');
#    console.error('Try:');
#    console.error('  1. Set NODE_PATH:');
#    console.error(`     NODE_PATH=__TARGET_BASE__ node -e "const m = require('${PROJECT_NAME}'); console.log(m.getBuildInfo())"`);
#    console.error('  2. Use absolute path:');
#    console.error(`     node -e "const m = require('__MAIN_MODULE_DIR__/launcher.js'); console.log(m.getBuildInfo())"`);
#    console.error('  3. Check TypeScript definitions:');
#    console.error(`     ls -la __MAIN_MODULE_DIR__/types/`);
#    console.error('  4. Reinstall:');
#    console.error(`     sudo apt reinstall __PROJECT_NAME_SANITIZED__-nodejs-__DB_BACKEND_LOWER__`);
#    console.error('');
#    process.exit(1);
#}
#LAUNCHER_EOF

#    # Replace placeholders in launcher.js
#    sed -i "s/__PROJECT_NAME__/$PROJECT_NAME/g" "$MAIN_MODULE_DIR/launcher.js"
#    sed -i "s/__PROJECT_NAME_SANITIZED__/$PROJECT_NAME_SANITIZED/g" "$MAIN_MODULE_DIR/launcher.js"
#    sed -i "s/__DB_BACKEND_LOWER__/$DB_BACKEND_LOWER/g" "$MAIN_MODULE_DIR/launcher.js"
#    sed -i "s|__TARGET_BASE__|$TARGET_BASE|g" "$MAIN_MODULE_DIR/launcher.js"
#    sed -i "s|__MAIN_MODULE_DIR__|$MAIN_MODULE_DIR|g" "$MAIN_MODULE_DIR/launcher.js"

#    chmod 755 "$MAIN_MODULE_DIR/launcher.js"

#    # Create main package.json with types field
#    cat > "$MAIN_MODULE_DIR/package.json" << PKG_EOF
#{
#  "name": "$PROJECT_NAME",
#  "version": "0.1.0",
#  "main": "./launcher.js",
#  "types": "./types/${PROJECT_NAME}.d.ts",
#  "description": "CAOS Native Node.js Bindings for $DB_BACKEND",
#  "engines": {
#    "node": ">=14.0.0"
#  }
#}
#PKG_EOF

#    # Copy tsconfig.json to main directory if it exists in version directory
#    if [ -f "$VERSION_MODULE_DIR/tsconfig.json" ] && [ ! -f "$MAIN_MODULE_DIR/tsconfig.json" ]; then
#        cp "$VERSION_MODULE_DIR/tsconfig.json" "$MAIN_MODULE_DIR/"
#    fi

#    ln -sf "launcher.js" "$MAIN_MODULE_DIR/index.js"

#    # Create symlink in /usr/local/lib/node_modules for global access
#    if [ "$TARGET_BASE" != "/usr/local/lib/node_modules" ] && [ -d "/usr/local/lib/node_modules" ]; then
#        if [ ! -L "/usr/local/lib/node_modules/$PROJECT_NAME" ] || [ ! -d "/usr/local/lib/node_modules/$PROJECT_NAME" ]; then
#            ln -sf "$MAIN_MODULE_DIR" "/usr/local/lib/node_modules/$PROJECT_NAME" 2>/dev/null || true
#            echo "  Created symlink in /usr/local/lib/node_modules/"
#        fi
#    fi

#    echo "  Installed Node.js module for version: $NODE_DIR_NAME"
#    echo "  TypeScript definitions: $MAIN_MODULE_DIR/types/${PROJECT_NAME}.d.ts"

#else
#    echo "  ERROR: Module directory not found: $VERSION_MODULE_DIR"
#    exit 1
#fi

#echo "Installation completed."
#EOF

#    sed -i "s|__VERSION_MODULE_DIR__|$VERSION_MODULE_DIR|g" "$deb_dir/DEBIAN/postinst"
#    sed -i "s|__MAIN_MODULE_DIR__|$MAIN_MODULE_DIR|g" "$deb_dir/DEBIAN/postinst"
#    sed -i "s/__PROJECT_NAME__/$PROJECT_NAME/g" "$deb_dir/DEBIAN/postinst"
#    sed -i "s/__NODE_DIR_NAME__/$node_dir_name/g" "$deb_dir/DEBIAN/postinst"
#    sed -i "s/__PROJECT_NAME_SANITIZED__/$PROJECT_NAME_SANITIZED/g" "$deb_dir/DEBIAN/postinst"
#    sed -i "s/__DB_BACKEND_LOWER__/$DB_BACKEND_LOWER/g" "$deb_dir/DEBIAN/postinst"
#    sed -i "s|__TARGET_BASE__|$TARGET_BASE|g" "$deb_dir/DEBIAN/postinst"

#    chmod +x "$deb_dir/DEBIAN/postinst"
#}

#create_version_prerm() {
#    local deb_dir=$1
#    local node_version=$2
#    local node_dir_name=$3

#    local TARGET_BASE=$(get_nodejs_target_dir)
#    local VERSION_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}/${node_dir_name}"
#    local MAIN_MODULE_DIR="$TARGET_BASE/${PROJECT_NAME}"

#    cat > "$deb_dir/DEBIAN/prerm" << 'EOF'
##!/bin/bash
#set -e

#VERSION_MODULE_DIR="__VERSION_MODULE_DIR__"
#MAIN_MODULE_DIR="__MAIN_MODULE_DIR__"
#PROJECT_NAME="__PROJECT_NAME__"

#case "$1" in
#    remove|purge)
#        echo "Removing ${PROJECT_NAME} module for version __NODE_DIR_NAME__..."

#        if [ -d "$VERSION_MODULE_DIR" ]; then
#            rm -rf "$VERSION_MODULE_DIR"
#            echo "Removed version directory: $VERSION_MODULE_DIR"
#        fi

#        # Only remove main directory if no versions left
#        if [ -d "$MAIN_MODULE_DIR" ]; then
#            remaining_versions=0
#            for dir in "$MAIN_MODULE_DIR"/v*; do
#                [ -d "$dir" ] || continue

#                if [ -d "$dir" ]; then
#                    remaining_versions=$((remaining_versions + 1))
#                fi
#            done

#            if [ "$remaining_versions" -eq 0 ]; then
#                rm -rf "$MAIN_MODULE_DIR"
#                echo "Removed main directory (no versions left)"

#                # Remove symlink if it exists
#                if [ -L "/usr/local/lib/node_modules/$PROJECT_NAME" ]; then
#                    rm -f "/usr/local/lib/node_modules/$PROJECT_NAME"
#                    echo "Removed symlink from /usr/local/lib/node_modules/"
#                fi
#            fi
#        fi
#        ;;
#esac
#EOF

#    sed -i "s|__VERSION_MODULE_DIR__|$VERSION_MODULE_DIR|g" "$deb_dir/DEBIAN/prerm"
#    sed -i "s|__MAIN_MODULE_DIR__|$MAIN_MODULE_DIR|g" "$deb_dir/DEBIAN/prerm"
#    sed -i "s/__PROJECT_NAME__/$PROJECT_NAME/g" "$deb_dir/DEBIAN/prerm"
#    sed -i "s/__NODE_DIR_NAME__/$node_dir_name/g" "$deb_dir/DEBIAN/prerm"

#    chmod +x "$deb_dir/DEBIAN/prerm"
#}

#create_version_control() {
#    local deb_dir=$1
#    local package_name=$2
#    local node_version=$3
#    local node_dir_name=$4

#    local conflicts_clause=""
#    case "${DB_BACKEND_LOWER}" in
#        "mysql")
#            conflicts_clause="${PROJECT_NAME_SANITIZED}-nodejs-mariadb-${node_dir_name}, ${PROJECT_NAME_SANITIZED}-nodejs-postgresql-${node_dir_name}"
#            ;;
#        "mariadb")
#            conflicts_clause="${PROJECT_NAME_SANITIZED}-nodejs-mysql-${node_dir_name}, ${PROJECT_NAME_SANITIZED}-nodejs-postgresql-${node_dir_name}"
#            ;;
#        "postgresql")
#            conflicts_clause="${PROJECT_NAME_SANITIZED}-nodejs-mysql-${node_dir_name}, ${PROJECT_NAME_SANITIZED}-nodejs-mariadb-${node_dir_name}"
#            ;;
#    esac

#    cat > "$deb_dir/DEBIAN/control" << EOF
#Package: $package_name
#Version: $VERSION
#Architecture: amd64
#Maintainer: CAOSDBA Development Team
#Description: ${PROJECT_NAME} - CAOSDBA extension for Node.js ${node_dir_name} with $DB_BACKEND_LOWER backend
# Native Node.js bindings for CAOSDBA database operations.
# Includes TypeScript definitions for type-safe development.
# Installs to: $(get_nodejs_target_dir)/${PROJECT_NAME}/${node_dir_name}/
#Depends: nodejs (>= ${node_version}.0.0), ${PACKAGE_NAME_BASE}
#Conflicts: $conflicts_clause
#Section: javascript
#Priority: optional
#EOF
#}

#build_single_package() {
#    local source_file=$1
#    local node_version=$2
#    local node_dir_name=$3
#    local build_number=$4

#    local safe_dir_name=$(echo "$node_dir_name" | tr '.' '_' | tr '-' '_')
#    local package_name="${PACKAGE_NAME_BASE}-${safe_dir_name}"

#    local deb_dir="$BUILD_DIR/deb-nodejs-${safe_dir_name}"

#    rm -rf "$deb_dir"
#    create_structure "$deb_dir" "$node_version" "$node_dir_name"
#    copy_files "$deb_dir" "$source_file" "$node_version" "$node_dir_name"

#    create_version_postinst "$deb_dir" "$node_version" "$node_dir_name"
#    create_version_prerm "$deb_dir" "$node_version" "$node_dir_name"
#    create_version_control "$deb_dir" "$package_name" "$node_version" "$node_dir_name"

#    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${package_name}_${VERSION}_amd64.deb" >/dev/null 2>&1

#    rm -rf "$deb_dir"
#    echo "Built: ${package_name}_${VERSION}_amd64.deb (Node.js ${node_dir_name})"
#    echo "  Installs to: $(get_nodejs_target_dir)/${PROJECT_NAME}/${node_dir_name}/"
#    echo "  TypeScript definitions included"
#    return 0
#}

#create_meta_package() {
#    local deb_dir="$BUILD_DIR/deb-meta-node"
#    rm -rf "$deb_dir"
#    mkdir -p "$deb_dir/DEBIAN"

#    # Collect available major versions
#    local all_versions=""
#    shopt -s nullglob
#    for pkg_file in "$BUILD_DIR"/*-nodejs-${DB_BACKEND_LOWER}-v*.deb; do
#        if [[ "$(basename "$pkg_file")" =~ ${PACKAGE_NAME_BASE}-v([0-9]+)_ ]]; then
#            local version="${BASH_REMATCH[1]}"
#            if [ -n "$all_versions" ]; then
#                all_versions="$all_versions, "
#            fi
#            all_versions="$all_versions$version"
#        fi
#    done
#    shopt -u nullglob

#    cat > "$deb_dir/DEBIAN/control" << EOF
#Package: ${PACKAGE_NAME_BASE}
#Version: $VERSION
#Architecture: all
#Maintainer: CAOSDBA Development Team
#Description: ${PROJECT_NAME} - CAOSDBA Node.js extension with $DB_BACKEND_LOWER backend (metapackage)
# This metapackage installs the appropriate Node.js version module.
# Includes TypeScript definitions for type-safe development.
# Available for Node.js versions: ${all_versions}
#Section: javascript
#Priority: optional
#Recommends: ${PACKAGE_NAME_BASE}-v18 | \
#            ${PACKAGE_NAME_BASE}-v20 | \
#            ${PACKAGE_NAME_BASE}-v22
#Conflicts: ${PROJECT_NAME_SANITIZED}-nodejs-mysql, ${PROJECT_NAME_SANITIZED}-nodejs-mariadb, ${PROJECT_NAME_SANITIZED}-nodejs-postgresql
#EOF

#    cat > "$deb_dir/DEBIAN/postinst" << 'EOF'
##!/bin/bash
#set -e

#echo "=========================================="
#echo "${PROJECT_NAME} Node.js Module Installed"
#echo "=========================================="
#echo ""
#echo "To use ${PROJECT_NAME} in your Node.js application:"
#echo ""
#echo "Method 1: Set NODE_PATH (recommended):"
#echo "  NODE_PATH=/usr/lib/x86_64-linux-gnu/nodejs node -e \"const m = require('${PROJECT_NAME}'); console.log(m.getBuildInfo())\""
#echo ""
#echo "Method 2: Use absolute path:"
#echo "  node -e \"const m = require('/usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/launcher.js'); console.log(m.getBuildInfo())\""
#echo ""
#echo "TypeScript Development:"
#echo "  TypeScript definitions are included in:"
#echo "  /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/types/${PROJECT_NAME}.d.ts"
#echo ""
#echo "  To validate types:"
#echo "  cd /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/"
#echo "  tsc --noEmit --project tsconfig.json"
#echo ""
#echo "=========================================="
#EOF

#    sed -i "s/\${PROJECT_NAME}/${PROJECT_NAME}/g" "$deb_dir/DEBIAN/postinst"
#    chmod +x "$deb_dir/DEBIAN/postinst"

#    dpkg-deb --build "$deb_dir" "$BUILD_DIR/${PACKAGE_NAME_BASE}_${VERSION}_all.deb" >/dev/null 2>&1
#    rm -rf "$deb_dir"

#    echo "Built meta-package: ${PACKAGE_NAME_BASE}_${VERSION}_all.deb"
#    echo "  Includes TypeScript definitions"
#}

#create_repository_scripts() {
#    mkdir -p "$REPO_DIR"

#    cat > "$REPO_DIR/update-repo.sh" << EOF
##!/bin/bash
#REPO_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
#CONF_DIR="\$REPO_DIR/conf"
#OVERRIDE_FILE="\$CONF_DIR/override"

#mkdir -p "\$CONF_DIR"
#> "\$OVERRIDE_FILE"

#for deb_file in "\$REPO_DIR"/*.deb; do
#    if [ -f "\$deb_file" ]; then
#        package_name=\$(dpkg-deb -f "\$deb_file" Package 2>/dev/null || basename "\$deb_file" | cut -d'_' -f1)
#        if [ -n "\$package_name" ]; then
#            echo "\$package_name optional nodejs" >> "\$OVERRIDE_FILE"
#        fi
#    fi
#done

#cd "\$REPO_DIR" || exit 1
#rm -f Release.gpg InRelease

#dpkg-scanpackages --multiversion . "\$OVERRIDE_FILE" > Packages
#gzip -k -f Packages

#cat > Release << RELEASE_CONTENT
#Origin: ${PROJECT_NAME} Node.js Repository
#Label: ${PROJECT_NAME}
#Suite: stable
#Codename: ${PROJECT_NAME}
#Version: 1.0
#Architectures: amd64 all
#Components: main
#Description: ${PROJECT_NAME} - CAOSDBA Node.js extension with TypeScript support
#Date: \$(date -Ru)
#MD5Sum:
# \$(md5sum Packages | cut -d' ' -f1) \$(stat -c %s Packages) Packages
# \$(md5sum Packages.gz | cut -d' ' -f1) \$(stat -c %s Packages.gz) Packages.gz
#SHA256:
# \$(sha256sum Packages | cut -d' ' -f1) \$(stat -c %s Packages) Packages
# \$(sha256sum Packages.gz | cut -d' ' -f1) \$(stat -c %s Packages.gz) Packages.gz
#RELEASE_CONTENT

#echo "Repository index updated"
#EOF
#    chmod +x "$REPO_DIR/update-repo.sh"

#    cat > "$REPO_DIR/setup-apt-source.sh" << EOF
##!/bin/bash
#if [ "\$EUID" -ne 0 ]; then
#    echo "ERROR: Run as root"
#    exit 1
#fi

#REPO_DIR="\$(cd "\$(dirname "\$0")" && pwd)"
#if [ ! -f "\$REPO_DIR/Packages.gz" ]; then
#    echo "ERROR: Run ./update-repo.sh first"
#    exit 1
#fi

#echo "deb [trusted=yes] file:\$REPO_DIR ./" > /etc/apt/sources.list.d/${PROJECT_NAME}-nodejs-local.list
#apt-get update -oAcquire::AllowInsecureRepositories=true

#echo ""
#echo "=== ${PROJECT_NAME} Repository Ready ==="
#echo ""
#echo "Install the meta-package:"
#echo "  sudo apt install ${PACKAGE_NAME_BASE}"
#echo ""
#echo "TypeScript definitions are included in the package."
#echo "Usage instructions in /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/launcher.js"
#EOF
#    chmod +x "$REPO_DIR/setup-apt-source.sh"
#}

#move_packages_to_repository() {
#    mkdir -p "$REPO_DIR"
#    local moved_count=0
#    for pkg_file in "$BUILD_DIR"/*.deb; do
#        if [ -f "$pkg_file" ]; then
#            mv "$pkg_file" "$REPO_DIR/"
#            moved_count=$((moved_count + 1))
#        fi
#    done
#    echo "Moved $moved_count packages to $REPO_DIR"
#}

#main() {
#    check_prerequisites
#    local ext_files=$(find_node_extension_files)
#    local built_count=0

#    while IFS= read -r source_file; do
#        [ -z "$source_file" ] && continue

#        local source_dir=$(dirname "$source_file")
#        local dir_name=$(basename "$source_dir")
#        local version_info=$(extract_node_version "$dir_name")

#        if [ -z "$version_info" ]; then
#            local parent_dir=$(dirname "$source_dir")
#            local parent_name=$(basename "$parent_dir")
#            version_info=$(extract_node_version "$parent_name")
#        fi

#        if [ -z "$version_info" ]; then
#            echo "WARNING: Cannot extract version from: $source_file"
#            continue
#        fi

#        IFS='|' read -r node_major node_dir_name <<< "$version_info"
#        if [ -z "$node_major" ] || [ -z "$node_dir_name" ]; then
#            echo "WARNING: Invalid version format: $version_info"
#            continue
#        fi

#        if build_single_package "$source_file" "$node_major" "$node_dir_name" "$(extract_build_number)"; then
#            built_count=$((built_count + 1))
#        fi
#    done <<< "$ext_files"

#    if [ "$built_count" -gt 0 ]; then
#        create_meta_package
#        move_packages_to_repository
#        create_repository_scripts

#        echo "SUCCESS: Built $built_count Node.js packages"
#        echo "Repository: $REPO_DIR"
#        echo "Test installation:"
#        echo "  sudo apt install ${PACKAGE_NAME_BASE}"
#        echo ""
#        echo "After installation, test with:"
#        echo "  NODE_PATH=/usr/lib/x86_64-linux-gnu/nodejs node -e \"const m = require('${PROJECT_NAME}'); console.log(m.getBuildInfo())\""
#        echo ""
#        echo "TypeScript development:"
#        echo "  TypeScript definitions installed to:"
#        echo "  /usr/lib/x86_64-linux-gnu/nodejs/${PROJECT_NAME}/types/${PROJECT_NAME}.d.ts"
#        return 0
#    else
#        echo "ERROR: No packages built"
#        return 1
#    fi
#}

#main "$@"
