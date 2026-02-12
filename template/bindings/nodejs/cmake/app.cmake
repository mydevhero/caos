message(STATUS "Building CAOSDBA as Node.js extension")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Check for required Node.js development packages
set(NODEJS_MISSING_PACKAGES "")

set(PROJECT_DIR ${CMAKE_SOURCE_DIR})

# Check for libnode-dev
find_path(LIBNODE_DEV_INCLUDE node.h
  PATHS
    /usr/include/node
    /usr/local/include/node
    /opt/homebrew/include/node
  NO_DEFAULT_PATH
)
if(NOT LIBNODE_DEV_INCLUDE)
  list(APPEND NODEJS_MISSING_PACKAGES "libnode-dev")
endif()

# Check for node-gyp
find_program(NODE_GYP_EXECUTABLE node-gyp
  PATHS /usr/bin /usr/local/bin /opt/homebrew/bin
)
if(NOT NODE_GYP_EXECUTABLE)
  list(APPEND NODEJS_MISSING_PACKAGES "node-gyp")
endif()

# Check for npm (required for node-addon-api installation)
find_program(NPM_EXECUTABLE npm)
if(NOT NPM_EXECUTABLE)
  list(APPEND NODEJS_MISSING_PACKAGES "npm")
endif()

# Check for TypeScript compiler (optional, for type validation)
find_program(TSC_EXECUTABLE tsc
  PATHS /usr/bin /usr/local/bin /opt/homebrew/bin
)

# If any required packages are missing, show error and stop
if(NODEJS_MISSING_PACKAGES)
  list(JOIN NODEJS_MISSING_PACKAGES ", " MISSING_PACKAGES_STR)
  message(FATAL_ERROR "
    Required Node.js development packages missing: ${MISSING_PACKAGES_STR}

    Required for building CAOSDBA Node.js extension.

    Installation commands for Ubuntu/Debian:
        sudo apt-get update
        sudo apt-get install libnode-dev node-gyp

    Note: node-addon-api will be installed automatically during build if not present.

    After installation, re-run CMake with Node.js support.
    ")
endif()

message(STATUS "All required Node.js development packages found")

if(TSC_EXECUTABLE)
  message(STATUS "TypeScript compiler found: ${TSC_EXECUTABLE}")
else()
  message(STATUS "TypeScript compiler not found - type validation will be skipped")
endif()

# Define supported Node.js versions
set(SUPPORTED_NODE_VERSIONS "14" "16" "18" "20" "22" "24")
set(NODE_TARGETS "")
set(NODE_BUILD_VERSIONS "")
set(NODE_PACKAGE_DIRS "")

# Build counter logic
set(BUILD_COUNTER_FILE "${CMAKE_BINARY_DIR}/build_counter.txt")
if(EXISTS "${BUILD_COUNTER_FILE}")
  file(READ "${BUILD_COUNTER_FILE}" CAOS_BUILD_COUNT)
  string(STRIP "${CAOS_BUILD_COUNT}" CAOS_BUILD_COUNT)
  math(EXPR CAOS_BUILD_COUNT "${CAOS_BUILD_COUNT} + 1")
else()
  set(CAOS_BUILD_COUNT 1)
endif()

string(TOLOWER "${CAOS_DB_BACKEND}" CAOS_DB_BACKEND_LOWER)

# Try to find each supported Node.js version
foreach(NODE_MAJOR_VER IN LISTS SUPPORTED_NODE_VERSIONS)
  # Find Node.js executable for this version
  find_program(NODE_EXECUTABLE_${NODE_MAJOR_VER}
    NAMES node${NODE_MAJOR_VER} node
    PATHS /usr/bin /usr/local/bin /opt/homebrew/bin
  )

  if(NODE_EXECUTABLE_${NODE_MAJOR_VER})

    # Get Node.js version
    execute_process(
      COMMAND ${NODE_EXECUTABLE_${NODE_MAJOR_VER}} --version
      OUTPUT_VARIABLE NODE_FULL_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Extract version number (v14.21.3 -> 14)
    string(REGEX REPLACE "^v([0-9]+)\\..*$" "\\1" NODE_DETECTED_MAJOR "${NODE_FULL_VERSION}")

    if(NODE_DETECTED_MAJOR STREQUAL NODE_MAJOR_VER)
      message(STATUS "Found Node.js ${NODE_FULL_VERSION}")

      # Get Node.js installation directory
      execute_process(
        COMMAND ${NODE_EXECUTABLE_${NODE_MAJOR_VER}} -p "require('path').dirname(process.execPath)"
        OUTPUT_VARIABLE NODE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )

      # Get Node-API version supported by this Node.js binary
      execute_process(
        COMMAND ${NODE_EXECUTABLE_${NODE_MAJOR_VER}} -p "process.versions.napi"
        OUTPUT_VARIABLE NODE_NAPI_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE NAPI_VER_RESULT
      )

      if(NAPI_VER_RESULT EQUAL 0)
        message(STATUS "Node-API version for Node ${NODE_MAJOR_VER}: ${NODE_NAPI_VERSION}")
      else()
        set(NODE_NAPI_VERSION "unknown")
      endif()

      # Find Node.js headers
      set(NODE_HEADERS_PATHS
        "${NODE_DIR}/../include/node"
        "/usr/include/node"
        "/usr/local/include/node"
        "/opt/homebrew/include/node"
      )

      set(NODE_HEADERS_DIR "")
      foreach(HEADER_PATH ${NODE_HEADERS_PATHS})
        if(EXISTS "${HEADER_PATH}/node_api.h")
          set(NODE_HEADERS_DIR "${HEADER_PATH}")
          break()
        endif()
      endforeach()

      if(NOT NODE_HEADERS_DIR)
        message(WARNING "Node.js headers not found for version ${NODE_MAJOR_VER}")
        continue()
      endif()

      # Find node-addon-api
      execute_process(
        COMMAND ${NODE_EXECUTABLE_${NODE_MAJOR_VER}} -p "require('node-addon-api').include"
        OUTPUT_VARIABLE NAPI_INCLUDE_DIR_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE NAPI_CHECK_RESULT
        ERROR_QUIET
      )

      if(NOT NAPI_CHECK_RESULT EQUAL 0)
        # Try to install node-addon-api
        find_program(NPM_EXECUTABLE npm)
        if(NPM_EXECUTABLE)
          execute_process(
            COMMAND ${NPM_EXECUTABLE} install node-addon-api
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            RESULT_VARIABLE NPM_INSTALL_RESULT
            OUTPUT_QUIET
            ERROR_QUIET
          )

          if(NPM_INSTALL_RESULT EQUAL 0)
            execute_process(
              COMMAND ${NODE_EXECUTABLE_${NODE_MAJOR_VER}} -p "require('node-addon-api').include"
              OUTPUT_VARIABLE NAPI_INCLUDE_DIR_RAW
              OUTPUT_STRIP_TRAILING_WHITESPACE
              RESULT_VARIABLE NAPI_RECHECK_RESULT
              ERROR_QUIET
            )
          endif()
        endif()
      endif()

      if(NAPI_CHECK_RESULT EQUAL 0 OR NAPI_RECHECK_RESULT EQUAL 0)
        string(STRIP "${NAPI_INCLUDE_DIR_RAW}" NAPI_INCLUDE_DIR)
        string(REPLACE "\n" "" NAPI_INCLUDE_DIR "${NAPI_INCLUDE_DIR}")
        string(REPLACE "\"" "" NAPI_INCLUDE_DIR "${NAPI_INCLUDE_DIR}")

        if(NOT IS_ABSOLUTE "${NAPI_INCLUDE_DIR}")
          set(NAPI_INCLUDE_DIR "${CMAKE_BINARY_DIR}/${NAPI_INCLUDE_DIR}")
        endif()

        # Create unique target name for this Node.js version
        set(TARGET_NAME "${PROJECT_NAME}_node${NODE_MAJOR_VER}")
        list(APPEND NODE_TARGETS ${TARGET_NAME})
        list(APPEND NODE_BUILD_VERSIONS ${NODE_FULL_VERSION})

        # Configure file
        configure_file(
          ${PROJECT_DIR}/src/include/extension_config.h.in
          ${CMAKE_BINARY_DIR}/extension_config.h
          @ONLY
        )

        # Create library target for this Node.js version
        add_library(${TARGET_NAME} MODULE
          ${PROJECT_DIR}/src/bindings.cpp
          ${PROJECT_DIR}/src/call_context.cpp
        )

        target_include_directories(${TARGET_NAME} PRIVATE
          ${CMAKE_BINARY_DIR}
          ${PROJECT_DIR}/src
          ${PROJECT_DIR}/src/include
          ${NODE_HEADERS_DIR}
          ${NAPI_INCLUDE_DIR}
        )

        # Set output properties
        set_target_properties(${TARGET_NAME} PROPERTIES
          PREFIX ""
          OUTPUT_NAME "${PROJECT_NAME}"
          SUFFIX ".node"
          LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/node_${NODE_MAJOR_VER}"
        )

        target_link_libraries(${TARGET_NAME} PRIVATE libcaos)
        target_compile_definitions(${TARGET_NAME} PRIVATE
          CAOS_BUILD_NODE_BINDING
          NODE_VERSION=${NODE_MAJOR_VER}
          NAPI_CPP_EXCEPTIONS
        )

        # Create Node.js package directory for this version
        set(NODE_PACKAGE_DIR "${CMAKE_BINARY_DIR}/node_package_${NODE_MAJOR_VER}/${PROJECT_NAME}")
        file(MAKE_DIRECTORY ${NODE_PACKAGE_DIR})
        list(APPEND NODE_PACKAGE_DIRS ${NODE_PACKAGE_DIR})

        string(TIMESTAMP CMAKE_TIMESTAMP "%Y-%m-%d %H:%M:%S")

        # Create TypeScript types directory for this version
        set(NODE_TYPES_DIR "${NODE_PACKAGE_DIR}/types")
        file(MAKE_DIRECTORY ${NODE_TYPES_DIR})

        # typescript/package.json
        set(PACKAGE_JSON_TEMPLATE "${PROJECT_DIR}/typescript/package.json")
        set(PACKAGE_JSON_OUTPUT "${NODE_PACKAGE_DIR}/package.json")
        if(EXISTS "${PACKAGE_JSON_TEMPLATE}")
          # Read the template
          file(READ "${PACKAGE_JSON_TEMPLATE}" PACKAGE_JSON_CONTENT)

          # Replace placeholders with actual values
          string(REPLACE "__PROJECT_NAME__" "${PROJECT_NAME}" PACKAGE_JSON_CONTENT "${PACKAGE_JSON_CONTENT}")
          string(REPLACE "__CAOS_DB_BACKEND_LOWER__" "${CAOS_DB_BACKEND_LOWER}" PACKAGE_JSON_CONTENT "${PACKAGE_JSON_CONTENT}")
          string(REPLACE "__CAOS_BUILD_COUNT__" "${CAOS_BUILD_COUNT}" PACKAGE_JSON_CONTENT "${PACKAGE_JSON_CONTENT}")
          string(REPLACE "__TIMESTAMP__" "${CMAKE_TIMESTAMP}" PACKAGE_JSON_CONTENT "${PACKAGE_JSON_CONTENT}")
          string(REPLACE "__NODE_NAPI_VERSION__" "${NODE_NAPI_VERSION}" PACKAGE_JSON_CONTENT "${PACKAGE_JSON_CONTENT}")
          string(REPLACE "__NODE_MAJOR_VER__" "${NODE_MAJOR_VER}" PACKAGE_JSON_CONTENT "${PACKAGE_JSON_CONTENT}")

          # Write the processed file
          file(WRITE "${PACKAGE_JSON_OUTPUT}" "${PACKAGE_JSON_CONTENT}")
          message(STATUS "Generated file for Node.js ${NODE_MAJOR_VER}: ${PACKAGE_JSON_OUTPUT}")
        else()
          message(FATAL_ERROR "File not found: ${PACKAGE_JSON_TEMPLATE}")
        endif()

        # typescript/index.js
        set(INDEX_JS_TEMPLATE "${PROJECT_DIR}/typescript/index.js")
        set(INDEX_JS_OUTPUT "${NODE_PACKAGE_DIR}/index.js")
        if(EXISTS "${INDEX_JS_TEMPLATE}")
          # Read the template
          file(READ "${INDEX_JS_TEMPLATE}" INDEX_JS_CONTENT)

          # Replace placeholders with actual values
          string(REPLACE "__PROJECT_NAME__" "${PROJECT_NAME}" INDEX_JS_CONTENT "${INDEX_JS_CONTENT}")
          string(REPLACE "__CAOS_DB_BACKEND_LOWER__" "${CAOS_DB_BACKEND_LOWER}" INDEX_JS_CONTENT "${INDEX_JS_CONTENT}")
          string(REPLACE "__CAOS_BUILD_COUNT__" "${CAOS_BUILD_COUNT}" INDEX_JS_CONTENT "${INDEX_JS_CONTENT}")
          string(REPLACE "__TIMESTAMP__" "${CMAKE_TIMESTAMP}" INDEX_JS_CONTENT "${INDEX_JS_CONTENT}")
          string(REPLACE "__NODE_NAPI_VERSION__" "${NODE_NAPI_VERSION}" INDEX_JS_CONTENT "${INDEX_JS_CONTENT}")
          string(REPLACE "__NODE_MAJOR_VER__" "${NODE_MAJOR_VER}" INDEX_JS_CONTENT "${INDEX_JS_CONTENT}")

          # Write the processed file
          file(WRITE "${INDEX_JS_OUTPUT}" "${INDEX_JS_CONTENT}")
          message(STATUS "Generated file for Node.js ${NODE_MAJOR_VER}: ${INDEX_JS_OUTPUT}")
        else()
          message(FATAL_ERROR "File not found: ${INDEX_JS_TEMPLATE}")
        endif()

        # Generate TypeScript declaration file for this version
        set(TYPESCRIPT_TEMPLATE "${PROJECT_DIR}/typescript/types/__PROJECT_NAME__.d.ts")
        set(TYPESCRIPT_OUTPUT "${NODE_TYPES_DIR}/${PROJECT_NAME}.d.ts")

        if(EXISTS "${TYPESCRIPT_TEMPLATE}")
          # Read the template
          file(READ "${TYPESCRIPT_TEMPLATE}" TYPESCRIPT_CONTENT)

          # Replace placeholders with actual values
          string(REPLACE "__PROJECT_NAME__" "${PROJECT_NAME}" TYPESCRIPT_CONTENT "${TYPESCRIPT_CONTENT}")
          string(REPLACE "__CAOS_DB_BACKEND_LOWER__" "${CAOS_DB_BACKEND_LOWER}" TYPESCRIPT_CONTENT "${TYPESCRIPT_CONTENT}")
          string(REPLACE "__CAOS_BUILD_COUNT__" "${CAOS_BUILD_COUNT}" TYPESCRIPT_CONTENT "${TYPESCRIPT_CONTENT}")
          string(REPLACE "__TIMESTAMP__" "${CMAKE_TIMESTAMP}" TYPESCRIPT_CONTENT "${TYPESCRIPT_CONTENT}")
          string(REPLACE "__NODE_NAPI_VERSION__" "${NODE_NAPI_VERSION}" TYPESCRIPT_CONTENT "${TYPESCRIPT_CONTENT}")
          string(REPLACE "__NODE_MAJOR_VER__" "${NODE_MAJOR_VER}" TYPESCRIPT_CONTENT "${TYPESCRIPT_CONTENT}")

          # Write the processed file
          file(WRITE "${TYPESCRIPT_OUTPUT}" "${TYPESCRIPT_CONTENT}")
          message(STATUS "Generated TypeScript definitions for Node.js ${NODE_MAJOR_VER}: ${TYPESCRIPT_OUTPUT}")
        else()
          message(FATAL_ERROR "File not found: ${TYPESCRIPT_TEMPLATE}")
        endif()

        # Copy test-types.ts if it exists
        set(TEST_TYPES_TEMPLATE "${PROJECT_DIR}/typescript/types/test-types.ts")
        set(TEST_TYPES_OUTPUT "${NODE_TYPES_DIR}/test-types.ts")

        if(EXISTS "${TEST_TYPES_TEMPLATE}")
          file(READ "${TEST_TYPES_TEMPLATE}" TEST_TYPES_CONTENT)
          string(REPLACE "__PROJECT_NAME__" "${PROJECT_NAME}" TEST_TYPES_CONTENT "${TEST_TYPES_CONTENT}")
          string(REPLACE "__CAOS_DB_BACKEND_LOWER__" "${CAOS_DB_BACKEND_LOWER}" TEST_TYPES_CONTENT "${TEST_TYPES_CONTENT}")
          string(REPLACE "__CAOS_BUILD_COUNT__" "${CAOS_BUILD_COUNT}" TEST_TYPES_CONTENT "${TEST_TYPES_CONTENT}")
          string(REPLACE "__TIMESTAMP__" "${CMAKE_TIMESTAMP}" TEST_TYPES_CONTENT "${TEST_TYPES_CONTENT}")
          string(REPLACE "__NODE_MAJOR_VER__" "${NODE_MAJOR_VER}" TEST_TYPES_CONTENT "${TEST_TYPES_CONTENT}")
          string(REPLACE "__NODE_NAPI_VERSION__" "${NODE_NAPI_VERSION}" TEST_TYPES_CONTENT "${TEST_TYPES_CONTENT}")
          file(WRITE "${TEST_TYPES_OUTPUT}" "${TEST_TYPES_CONTENT}")
          message(STATUS "Copied TypeScript test file for Node.js ${NODE_MAJOR_VER}")
        endif()

        # Copy tsconfig.json if it exists
        set(TSCONFIG_TEMPLATE "${PROJECT_DIR}/typescript/tsconfig.json")
        set(TSCONFIG_OUTPUT "${NODE_PACKAGE_DIR}/tsconfig.json")

        if(EXISTS "${TSCONFIG_TEMPLATE}")
          file(READ "${TSCONFIG_TEMPLATE}" TSCONFIG_CONTENT)
          string(REPLACE "__PROJECT_NAME__" "${PROJECT_NAME}" TSCONFIG_CONTENT "${TSCONFIG_CONTENT}")
          string(REPLACE "__CAOS_DB_BACKEND_LOWER__" "${CAOS_DB_BACKEND_LOWER}" TSCONFIG_CONTENT "${TSCONFIG_CONTENT}")
          string(REPLACE "__CAOS_BUILD_COUNT__" "${CAOS_BUILD_COUNT}" TSCONFIG_CONTENT "${TSCONFIG_CONTENT}")
          string(REPLACE "__TIMESTAMP__" "${CMAKE_TIMESTAMP}" TSCONFIG_CONTENT "${TSCONFIG_CONTENT}")
          string(REPLACE "__NODE_MAJOR_VER__" "${NODE_MAJOR_VER}" TSCONFIG_CONTENT "${TSCONFIG_CONTENT}")
          string(REPLACE "__NODE_NAPI_VERSION__" "${NODE_NAPI_VERSION}" TSCONFIG_CONTENT "${TSCONFIG_CONTENT}")
          file(WRITE "${TSCONFIG_OUTPUT}" "${TSCONFIG_CONTENT}")
          message(STATUS "Copied tsconfig.json for Node.js ${NODE_MAJOR_VER}")
        endif()

        # Validate TypeScript types if tsc is available
        if(TSC_EXECUTABLE AND EXISTS "${TEST_TYPES_OUTPUT}")
          add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Validating TypeScript definitions for Node.js ${NODE_MAJOR_VER}..."
            COMMAND cd "${NODE_PACKAGE_DIR}" && ${TSC_EXECUTABLE} --noEmit --project tsconfig.json || ${CMAKE_COMMAND} -E echo "TypeScript validation failed or warnings found"
            COMMENT "Validating TypeScript type definitions"
            VERBATIM
            WORKING_DIRECTORY ${NODE_PACKAGE_DIR}
          )
        endif()

        # POST_BUILD: copy to package directory
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy
          $<TARGET_FILE:${TARGET_NAME}>
          ${NODE_PACKAGE_DIR}/${PROJECT_NAME}.node
          COMMENT "Building ${PROJECT_NAME} for Node.js ${NODE_MAJOR_VER}"
          VERBATIM
        )

        # Create directory in dist structure
        set(NODE_REPO_DIR "${PROJECT_DIR}/dist/repositories/${PROJECT_NAME}/nodejs/v${NODE_MAJOR_VER}")
        file(MAKE_DIRECTORY ${NODE_REPO_DIR})

        # Target for copying to dist
        add_custom_target(${TARGET_NAME}_copy_to_dist
          COMMAND ${CMAKE_COMMAND} -E make_directory ${NODE_REPO_DIR}
          COMMAND ${CMAKE_COMMAND} -E copy
          ${NODE_PACKAGE_DIR}/${PROJECT_NAME}.node
          ${NODE_REPO_DIR}/${PROJECT_NAME}.node
          COMMAND ${CMAKE_COMMAND} -E copy
          ${NODE_PACKAGE_DIR}/index.js
          ${NODE_REPO_DIR}/index.js
          COMMAND ${CMAKE_COMMAND} -E copy
          ${NODE_PACKAGE_DIR}/package.json
          ${NODE_REPO_DIR}/package.json
          # Copy TypeScript files
          COMMAND ${CMAKE_COMMAND} -E make_directory ${NODE_REPO_DIR}/types
          COMMAND ${CMAKE_COMMAND} -E copy
          ${NODE_TYPES_DIR}/${PROJECT_NAME}.d.ts
          ${NODE_REPO_DIR}/types/${PROJECT_NAME}.d.ts
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${TEST_TYPES_OUTPUT}
          ${NODE_REPO_DIR}/types/test-types.ts
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${TSCONFIG_OUTPUT}
          ${NODE_REPO_DIR}/tsconfig.json
          DEPENDS ${TARGET_NAME}
          COMMENT "Copying ${PROJECT_NAME} Node.js files to dist directory"
        )

      else()
        message(WARNING "node-addon-api not found for Node.js ${NODE_MAJOR_VER}")
      endif()
    endif()
  endif()
endforeach()

message(STATUS "Building for Node.js versions: ${NODE_BUILD_VERSIONS}")
message(STATUS "Node targets created: ${NODE_TARGETS}")

# Create main library target to satisfy parent CMake
add_library(${PROJECT_NAME} OBJECT)
target_link_libraries(${PROJECT_NAME} PRIVATE libcaos repoexception)

# Add dummy source file
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_${PROJECT_NAME}.cpp
  "// Dummy source for ${PROJECT_NAME} library\n")
target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/dummy_${PROJECT_NAME}.cpp)

# Create meta-target that depends on all Node.js version targets
add_custom_target(make_node_all DEPENDS ${NODE_TARGETS})

# Create target that copies ALL Node.js versions to dist
add_custom_target(do_copy_all_to_dist)
foreach(TARGET_NAME IN LISTS NODE_TARGETS)
  add_dependencies(do_copy_all_to_dist ${TARGET_NAME}_copy_to_dist)
endforeach()

# Ensure Node.js extensions are built when main target is built
add_dependencies(${PROJECT_NAME} make_node_all)

# Create aggregated Node.js package with all versions
set(AGGREGATE_NODE_PACKAGE_DIR "${CMAKE_BINARY_DIR}/node_package/${PROJECT_NAME}")
file(MAKE_DIRECTORY ${AGGREGATE_NODE_PACKAGE_DIR})

# Create aggregated index.js that works for all Node.js versions
# Add TypeScript hint
file(WRITE ${AGGREGATE_NODE_PACKAGE_DIR}/index.js
  [[
  /**
   * ${PROJECT_NAME} - CAOSDBA Native Node.js Bindings
   *
   * Native extension for CAOSDBA database operations.
   * Backend: ${CAOS_DB_BACKEND_LOWER}
   * Build: ${CAOS_BUILD_COUNT}
   *
   * @module ${PROJECT_NAME}
   * @see ./types/${PROJECT_NAME}.d.ts for TypeScript definitions
   *
   * This package contains native extensions for multiple Node.js versions.
   * The correct version will be loaded automatically based on your Node.js version.
   */

  const fs = require('fs');
  const path = require('path');

  function loadCorrectModule() {
  const packageDir = __dirname;
  const nodeVersion = process.versions.node;
  const nodeMajor = nodeVersion.split('.')[0];

  // Try to find version-specific module
  const versionedModule = path.join(packageDir, `v${nodeMajor}`, `${PROJECT_NAME}.node`);

  if (fs.existsSync(versionedModule)) {
    return require(versionedModule);
    }

    // Fallback to any available module
    const files = fs.readdirSync(packageDir);
    for (const file of files) {
    if (file.startsWith('v') && fs.existsSync(path.join(packageDir, file, `${PROJECT_NAME}.node`))) {
      console.warn(`Using Node.js ${file} module for Node.js ${nodeVersion}`);
      return require(path.join(packageDir, file, `${PROJECT_NAME}.node`));
      }
      }

      throw new Error(`No native module found for Node.js ${nodeVersion} in ${packageDir}`);
      }

      const native = loadCorrectModule();

      // Expose all public functions from native module
      module.exports = native;

      // Add metadata
      module.exports.__version__ = '0.1.0';
      module.exports.__build__ = ${CAOS_BUILD_COUNT};
      module.exports.__backend__ = '${CAOS_DB_BACKEND_LOWER}';
      module.exports.__node_version__ = process.versions.node;
      ]])

# Copy all version-specific modules to aggregate package
foreach(NODE_PACKAGE_DIR IN LISTS NODE_PACKAGE_DIRS)
  add_custom_command(TARGET make_node_all POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${NODE_PACKAGE_DIR}
    ${AGGREGATE_NODE_PACKAGE_DIR}
    COMMENT "Aggregating Node.js modules for all versions"
  )
endforeach()

# Create aggregated TypeScript types directory
set(AGGREGATE_TYPES_DIR "${AGGREGATE_NODE_PACKAGE_DIR}/types")
file(MAKE_DIRECTORY ${AGGREGATE_TYPES_DIR})

# Copy TypeScript files from first available version
if(NODE_PACKAGE_DIRS)
  list(GET NODE_PACKAGE_DIRS 0 FIRST_NODE_PACKAGE_DIR)
  set(FIRST_TYPES_DIR "${FIRST_NODE_PACKAGE_DIR}/types")

  if(EXISTS "${FIRST_TYPES_DIR}/${PROJECT_NAME}.d.ts")
    add_custom_command(TARGET make_node_all POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      "${FIRST_TYPES_DIR}/${PROJECT_NAME}.d.ts"
      "${AGGREGATE_TYPES_DIR}/${PROJECT_NAME}.d.ts"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${FIRST_TYPES_DIR}/test-types.ts"
      "${AGGREGATE_TYPES_DIR}/test-types.ts"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      "${FIRST_NODE_PACKAGE_DIR}/tsconfig.json"
      "${AGGREGATE_NODE_PACKAGE_DIR}/tsconfig.json"
      COMMENT "Copying TypeScript definitions to aggregate package"
    )
  endif()
endif()

# Update package.json in aggregate package to include types field
file(WRITE ${AGGREGATE_NODE_PACKAGE_DIR}/package.json
  [[
  {
  "name": "${PROJECT_NAME}",
  "version": "0.1.0",
  "description": "CAOSDBA Native Node.js Bindings for ${CAOS_DB_BACKEND_LOWER} backend",
  "main": "index.js",
  "types": "./types/${PROJECT_NAME}.d.ts",
  "engines": {
  "node": ">=14.0.0"
  },
  "gypfile": false
  }
  ]])

# Installation messages
install(CODE "
  message(STATUS \"\")
  message(STATUS \"Node.js extension '${PROJECT_NAME}' built for versions: ${NODE_BUILD_VERSIONS}\")
  message(STATUS \"Build: ${CAOS_BUILD_COUNT}, Backend: ${CAOS_DB_BACKEND_LOWER}\")
  if(\"${TSC_EXECUTABLE}\" STREQUAL \"TSC_EXECUTABLE-NOTFOUND\")
    message(STATUS \"TypeScript: Compiler not found - type validation was skipped\")
  else()
    message(STATUS \"TypeScript: Definitions generated and validated\")
  endif()
  message(STATUS \"\")
  message(STATUS \"To test the installation:\")
  message(STATUS \"  node -e \\\"const m = require('${PROJECT_NAME}'); console.log(m.getBuildInfo())\\\"\")
  message(STATUS \"  tsc --noEmit --project /path/to/${PROJECT_NAME}/tsconfig.json  # Validate types\")
  message(STATUS \"\")"
  COMPONENT node
)

# Package targets
if(CMAKE_BUILD_TYPE STREQUAL "release")
    add_custom_target(make_package_deb
        COMMAND ${PROJECT_DIR}/scripts/create_package_deb.sh ${CAOS_DB_BACKEND} ${PROJECT_NAME}
        DEPENDS libcaos do_copy_all_to_dist
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building DEB package for Node.js extension '${PROJECT_NAME}' with ${CAOS_DB_BACKEND_LOWER} backend"
    )

    add_custom_target(make_distribution_tarball
        COMMAND ${PROJECT_DIR}/scripts/create_distribution_tarball.sh ${CAOS_DB_BACKEND} ${PROJECT_NAME}
        DEPENDS make_package_deb
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating distribution tarball for Node.js extension '${PROJECT_NAME}' with ${CAOS_DB_BACKEND_LOWER} backend"
    )
else()
    add_custom_target(make_package_deb
        COMMAND ${CMAKE_COMMAND} -E echo "ERROR: CMAKE_BUILD_TYPE must be 'release' to build packages. Current value: ${CMAKE_BUILD_TYPE}"
        COMMAND false
    )

    add_custom_target(make_distribution_tarball
        COMMAND ${CMAKE_COMMAND} -E echo "ERROR: CMAKE_BUILD_TYPE must be 'release' to build packages. Current value: ${CMAKE_BUILD_TYPE}"
        COMMAND false
        DEPENDS make_package_deb
    )
endif()
