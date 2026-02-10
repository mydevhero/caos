#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../" && pwd)"
#BUILD_DIR="$PROJECT_ROOT/CAOSDBA/build/release"
BUILD_DIR="$PROJECT_ROOT/build/release"
DIST_DIR="$PROJECT_ROOT/dist"

DB_BACKEND=${1:-"MYSQL"}
PROJECT_NAME=${2:-"caos"}
PROJECT_NAME_SANITIZED=$(echo "$PROJECT_NAME" | tr '_' '-')
DB_BACKEND_LOWER=$(echo "$DB_BACKEND" | tr '[:upper:]' '[:lower:]')

if [ -f "$BUILD_DIR/build_counter.txt" ]; then
    CAOS_BUILD_COUNTER=$(cat "$BUILD_DIR/build_counter.txt")
    VERSION="1.0.0+${CAOS_BUILD_COUNTER}"
else
    VERSION="1.0.0+1"
    echo "WARNING: build_counter.txt not found, using default version"
fi

echo "Creating DEB packages tarball for ${PROJECT_NAME} PHP extension"
echo "Version: $VERSION"
echo "Database backend: $DB_BACKEND_LOWER"

check_prerequisites() {
    local repo_dir="$DIST_DIR/repositories/${PROJECT_NAME}/php"

    if [ ! -d "$repo_dir" ]; then
        echo "ERROR: PHP repository directory not found in $repo_dir"
        echo "Please run the ${PROJECT_NAME}_package_deb target first:"
        echo "  cmake --build build/release --target ${PROJECT_NAME}_package_deb"
        exit 1
    fi

    local deb_count=$(ls "$repo_dir"/*.deb 2>/dev/null | wc -l)
    if [ "$deb_count" -eq 0 ]; then
        echo "ERROR: No PHP DEB packages found in $repo_dir"
        echo "Please run the ${PROJECT_NAME}_package_deb target first"
        exit 1
    fi

    echo "Found $deb_count PHP DEB package(s) in nested repository"
}

create_tarball_structure() {
    local temp_dir="$DIST_DIR/temp_caos_php_deb"
    local project_repo_dir="$temp_dir/opt/caosdba/repositories/${PROJECT_NAME}/php"

    echo "Creating tarball structure in $temp_dir"

    rm -rf "$temp_dir"
    mkdir -p "$project_repo_dir"

    local source_repo_dir="$DIST_DIR/repositories/${PROJECT_NAME}/php"
    if [ -d "$source_repo_dir" ]; then
        echo "Copying PHP repository from $source_repo_dir"
        cp -r "$source_repo_dir"/* "$project_repo_dir/"
    else
        echo "ERROR: $source_repo_dir not found"
        exit 1
    fi

    if [ -f "$DIST_DIR/install-repository.sh" ]; then
        echo "Copying install-repository.sh from $DIST_DIR"
        mkdir -p "$temp_dir/opt/caosdba"
        cp "$DIST_DIR/install-repository.sh" "$temp_dir/opt/caosdba/"
    fi

    if [ -f "$DIST_DIR/README-PHP.md" ]; then
        echo "Copying README-PHP.md from $DIST_DIR"
        mkdir -p "$temp_dir/opt/caosdba"
        cp "$DIST_DIR/README-PHP.md" "$temp_dir/opt/caosdba/"
    elif [ -f "$DIST_DIR/README.md" ]; then
        echo "Copying README.md from $DIST_DIR"
        mkdir -p "$temp_dir/opt/caosdba"
        cp "$DIST_DIR/README.md" "$temp_dir/opt/caosdba/"
    fi

    if [ -f "$DIST_DIR/ENV.md" ]; then
        echo "Copying ENV.md from $DIST_DIR"
        mkdir -p "$temp_dir/opt/caosdba"
        cp "$DIST_DIR/ENV.md" "$temp_dir/opt/caosdba/"
    fi

    chown -R root:root "$temp_dir"
    chmod 755 "$temp_dir/opt/caosdba" 2>/dev/null || true
    find "$temp_dir" -type d -exec chmod 755 {} \;
    find "$temp_dir" -type f -exec chmod 644 {} \;

    chmod 755 "$temp_dir/opt/caosdba/install-repository.sh" 2>/dev/null || true
    chmod 755 "$project_repo_dir/update-repo.sh" 2>/dev/null || true
    chmod 755 "$project_repo_dir/setup-apt-source.sh" 2>/dev/null || true

    echo "PHP tarball structure created"
}

create_install_script() {
    local temp_dir="$DIST_DIR/temp_caos_php_deb"

    mkdir -p "$temp_dir/opt/caosdba"

    cat > "$temp_dir/opt/caosdba/install-repository.sh" << EOF
#!/bin/bash
# ${PROJECT_NAME} PHP Repository Installation Script

set -e

CAOS_OPT_DIR="/opt/caosdba"
REPO_DIR="\$CAOS_OPT_DIR/repositories/${PROJECT_NAME}/php"

if [ "\$EUID" -ne 0 ]; then
    echo "ERROR: This script must be run as root (use sudo)"
    exit 1
fi

echo "================================================================"
echo "${PROJECT_NAME} PHP Repository Installation"
echo "================================================================"

if [ ! -d "\$REPO_DIR" ]; then
    echo "ERROR: PHP repository directory not found: \$REPO_DIR"
    echo "Make sure the tarball was extracted to /opt/caosdba"
    exit 1
fi

echo "Setting up ${PROJECT_NAME} PHP repository from \$REPO_DIR"

# Clean up any existing PHP repository configurations
echo "Cleaning up existing PHP repository configurations..."
for file in /etc/apt/sources.list.d/${PROJECT_NAME}-php-local.list; do
    if [ -f "\$file" ]; then
        echo "  Removing: \$(basename "\$file")"
        rm -f "\$file"
    fi
done

echo "Updating PHP repository index..."
cd "\$REPO_DIR"
if [ -f "./update-repo.sh" ]; then
    ./update-repo.sh
else
    echo "ERROR: update-repo.sh not found in \$REPO_DIR"
    exit 1
fi

echo "Configuring APT repository for PHP..."
if [ -f "./setup-apt-source.sh" ]; then
    ./setup-apt-source.sh
else
    echo "ERROR: setup-apt-source.sh not found in \$REPO_DIR"
    exit 1
fi

echo ""
echo "================================================================"
echo "${PROJECT_NAME} PHP Repository Installation Completed"
echo "================================================================"
echo ""
echo "You can now install ${PROJECT_NAME} PHP extension:"
echo "  sudo apt install ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}"
echo ""
echo "Or for specific PHP version:"
echo "  sudo apt install ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-8.3"
echo ""
echo "To test the installation:"
echo "  php -m | grep ${PROJECT_NAME}"
echo ""
echo "PHP repository location: \$REPO_DIR"
echo ""
EOF

    chmod 755 "$temp_dir/opt/caosdba/install-repository.sh"
    echo "Created PHP install-repository.sh in tarball structure"
}

create_readme() {
    local readme_file="$DIST_DIR/README-PHP.md"
    local tarball_name="${PROJECT_NAME_SANITIZED}-php-deb-repository-${DB_BACKEND_LOWER}-${VERSION}.tar.gz"

    cat > "$readme_file" << EOF
# ${PROJECT_NAME} PHP DEB Packages Repository

Version: $VERSION
Database Backend: $DB_BACKEND
Project: $PROJECT_NAME

## Contents

This tarball contains the ${PROJECT_NAME} PHP DEB packages and local APT repository.

### Directory Structure

/opt/caosdba/
├── repositories/
│   └── ${PROJECT_NAME}/
│       └── php/              # PHP-specific repository
│           ├── *.deb         # PHP DEB packages
│           ├── Packages      # Repository index
│           ├── Packages.gz   # Compressed index
│           ├── Release       # Release info
│           ├── update-repo.sh  # PHP repository update script
│           └── setup-apt-source.sh # PHP APT configuration script
└── install-repository.sh     # Main installation script

## Installation

### Quick Installation

1. Extract the tarball to /opt:
\`\`\`bash
sudo tar -xzf ${tarball_name} -C /
\`\`\`

2. Run the installation script:
\`\`\`bash
sudo /opt/caosdba/install-repository.sh
\`\`\`

### Manual Installation

If you prefer manual setup:

1. Extract tarball:
\`\`\`bash
sudo tar -xzf ${tarball_name} -C /
\`\`\`

2. Update PHP repository index:
\`\`\`bash
cd /opt/caosdba/repositories/${PROJECT_NAME}/php
./update-repo.sh
\`\`\`

3. Configure APT for PHP:
\`\`\`bash
./setup-apt-source.sh
\`\`\`

## Usage

After installation, you can install ${PROJECT_NAME} PHP extension via APT:

\`\`\`bash
# Install PHP meta-package (recommended)
sudo apt install ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}

# Install specific PHP version
sudo apt install ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-8.3

# See all available PHP packages
apt list ${PROJECT_NAME_SANITIZED}-php*
\`\`\`

## Available PHP Packages

- ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER} - PHP meta-package
- ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-8.0 - PHP 8.0 extension
- ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-8.1 - PHP 8.1 extension
- ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-8.2 - PHP 8.2 extension
- ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-8.3 - PHP 8.3 extension
- ${PROJECT_NAME_SANITIZED}-php-${DB_BACKEND_LOWER}-8.4 - PHP 8.4 extension

## Testing

To verify the PHP installation:

\`\`\`bash
php -m | grep ${PROJECT_NAME}
\`\`\`

For specific PHP version:

\`\`\`bash
php8.3 -m | grep ${PROJECT_NAME}
\`\`\`

## Maintenance

To update the PHP repository after adding new packages:
\`\`\`bash
cd /opt/caosdba/repositories/${PROJECT_NAME}/php
./update-repo.sh
sudo apt update
\`\`\`

## Uninstallation

To remove the PHP repository:

1. Remove PHP APT source:
\`\`\`bash
sudo rm -f /etc/apt/sources.list.d/${PROJECT_NAME}-php-local.list
\`\`\`

2. Remove PHP repository files:
\`\`\`bash
sudo rm -rf /opt/caosdba/repositories/${PROJECT_NAME}/php
\`\`\`

3. Update APT cache:
\`\`\`bash
sudo apt update
\`\`\`

## Multiple Projects and Languages Support

The system supports multiple CAOS-based projects with different languages:

\`\`\`
/opt/caosdba/repositories/my_app/python/      # Python extension
/opt/caosdba/repositories/my_app/php/         # PHP extension
/opt/caosdba/repositories/another_app/python/ # Another project Python
/opt/caosdba/repositories/another_app/php/    # Another project PHP
\`\`\`

Each language has its own isolated repository with separate APT configuration.

---
Generated: $(date)
Build: $VERSION
Database: $DB_BACKEND
Project: $PROJECT_NAME
Language: PHP
EOF

    echo "Created README-PHP.md"
}

create_env_documentation() {
    local env_file="$DIST_DIR/ENV.md"

    cat > "$env_file" << EOF
# ${PROJECT_NAME} Environment Variables

This document describes all environment variables used by the ${PROJECT_NAME} PHP extension.

## Cache Configuration (Redis)

| Variable Name | Description | Default | Example |
|---------------|-------------|---------|---------|
| \`CAOS_CACHEUSER\` | Username for Redis 6+ with ACL support | - | \`myuser\` |
| \`CAOS_CACHEPASS\` | Authentication password for Redis | - | \`mypassword\` |
| \`CAOS_CACHEHOST\` | Redis server address | \`localhost\` | \`redis.example.com\` |
| \`CAOS_CACHEPORT\` | Redis server port number | \`6379\` | \`6380\` |
| \`CAOS_CACHECLIENTNAME\` | Client name identifier for Redis | - | \`myapp-client\` |
| \`CAOS_CACHEINDEX\` | Redis database number (0-15) | \`0\` | \`1\` |
| \`CAOS_CACHECOMMANDTIMEOUT\` | Timeout for Redis commands (seconds) | \`30\` | \`60\` |
| \`CAOS_CACHEPOOLSIZEMIN\` | Minimum number of connections always kept idle | \`5\` | \`10\` |
| \`CAOS_CACHEPOOLSIZEMAX\` | Maximum number of idle connections in the pool | \`20\` | \`50\` |
| \`CAOS_CACHEPOOLWAIT\` | Timeout when waiting for connection from exhausted pool (seconds) | \`30\` | \`60\` |
| \`CAOS_CACHEPOOLCONNECTIONTIMEOUT\` | Timeout for establishing connection (seconds) | \`10\` | \`30\` |
| \`CAOS_CACHEPOOLCONNECTIONLIFETIME\` | Absolute maximum lifetime of a connection (seconds) | \`3600\` | \`7200\` |
| \`CAOS_CACHEPOOLCONNECTIONIDLETIME\` | Maximum inactivity duration before closing connection (seconds) | \`300\` | \`600\` |

## Database Configuration

| Variable Name | Description | Default | Example |
|---------------|-------------|---------|---------|
| \`CAOS_DBUSER\` | Database username | - | \`dbuser\` |
| \`CAOS_DBPASS\` | Database password | - | \`dbpassword\` |
| \`CAOS_DBHOST\` | Database server address | \`localhost\` | \`db.example.com\` |
| \`CAOS_DBPORT\` | Database server port | \`3306\` | \`5432\` |
| \`CAOS_DBNAME\` | Database name | - | \`myapp_db\` |
| \`CAOS_DBPOOLSIZEMIN\` | Minimum number of database connections in pool | \`5\` | \`10\` |
| \`CAOS_DBPOOLSIZEMAX\` | Maximum number of database connections in pool | \`20\` | \`50\` |
| \`CAOS_DBPOOLWAIT\` | Timeout when waiting for database connection (seconds) | \`30\` | \`60\` |
| \`CAOS_DBPOOLTIMEOUT\` | Database connection pool timeout (seconds) | \`30\` | \`60\` |
| \`CAOS_DBCONNECT_TIMEOUT\` | Database connection timeout (seconds) | \`10\` | \`30\` |
| \`CAOS_DBMAXWAIT\` | Maximum wait time for database operations (seconds) | \`30\` | \`60\` |
| \`CAOS_DBHEALTHCHECKINTERVAL\` | Health check interval for database connections (seconds) | \`30\` | \`60\` |
| \`CAOS_LOG_THRESHOLD_CONNECTION_LIMIT_EXCEEDED\` | Log threshold for connection limit exceeded events | - | \`WARNING\` |
| \`CAOS_VALIDATE_CONNECTION_BEFORE_ACQUIRE\` | Validate connection before acquiring from pool (\`true\`/\`false\`) | \`true\` | \`false\` |
| \`CAOS_VALIDATE_USING_TRANSACTION\` | Validate connection using transaction (\`true\`/\`false\`) | \`false\` | \`true\` |

## Usage Examples

### Basic Redis Configuration

\`\`\`bash
export CAOS_CACHEHOST="redis.example.com"
export CAOS_CACHEPASS="secure_password"
export CAOS_CACHEPOOLSIZEMIN="10"
export CAOS_CACHEPOOLSIZEMAX="50"
export CAOS_CACHEPOOLCONNECTIONLIFETIME="7200"
export CAOS_CACHEPOOLCONNECTIONIDLETIME="600"
\`\`\`

### Database Configuration

\`\`\`bash
export CAOS_DBHOST="mysql.example.com"
export CAOS_DBUSER="app_user"
export CAOS_DBPASS="db_password"
export CAOS_DBNAME="application_db"
export CAOS_DBPOOLSIZEMIN="5"
export CAOS_DBPOOLSIZEMAX="20"
export CAOS_DBCONNECT_TIMEOUT="30"
\`\`\`

### Docker/Container Environment

\`\`\`bash
# Set in your Dockerfile or docker-compose.yml
ENV CAOS_CACHEHOST=redis
ENV CAOS_CACHEPORT=6379
ENV CAOS_DBHOST=mysql
ENV CAOS_DBUSER=app
ENV CAOS_DBPASS=password
ENV CAOS_DBNAME=app_db
\`\`\`

## Notes

Security: Never commit passwords to version control. Use environment-specific configuration files or secret management systems.

Performance: Adjust pool sizes based on your application's concurrency requirements.

Timeouts: Set appropriate timeouts based on your network latency and application requirements.

Validation: Connection validation adds overhead but improves reliability in unstable network environments.

For more information, refer to the ${PROJECT_NAME} documentation.
EOF

    echo "Created environment documentation: $env_file"
}

create_tarball() {
    local temp_dir="$DIST_DIR/temp_caos_php_deb"
    local tarball_name="${PROJECT_NAME_SANITIZED}-php-deb-repository-${DB_BACKEND_LOWER}-${VERSION}.tar.gz"
    local tarball_path="$DIST_DIR/$tarball_name"

    echo "Creating PHP tarball: $tarball_name"

    mkdir -p "$DIST_DIR"
    tar -czf "$tarball_path" -C "$temp_dir" opt

    local repo_dir="$temp_dir/opt/caosdba/repositories/${PROJECT_NAME}/php"
    local deb_count=$(ls "$repo_dir"/*.deb 2>/dev/null | wc -l)

    rm -rf "$temp_dir"

    echo "PHP tarball created: $tarball_path"
    echo "Contains $deb_count PHP DEB package(s)"
    echo ""
    echo "To deploy:"
    echo "  sudo tar -xzf $tarball_path -C /"
    echo "  sudo /opt/caosdba/install-repository.sh"
    echo ""
}

main() {
    echo "Starting PHP DEB repository tarball creation for $PROJECT_NAME..."

    check_prerequisites
    create_tarball_structure
    create_install_script
    create_readme
    create_env_documentation
    create_tarball

    echo "PHP DEB repository tarball creation completed successfully"
}

main "$@"
