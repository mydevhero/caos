message(STATUS "Building CAOS as Python extension")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_DIR ${CMAKE_SOURCE_DIR})

# Define supported Python versions
set(SUPPORTED_PYTHON_VERSIONS "3.8" "3.9" "3.10" "3.11" "3.12")
set(PYTHON_TARGETS "")
set(PYTHON_BUILD_VERSIONS "")
set(PYTHON_PACKAGE_DIRS "")

# Build counter logic - must be before Python detection for consistency
set(BUILD_COUNTER_FILE "${CMAKE_BINARY_DIR}/build_counter.txt")
if(EXISTS "${BUILD_COUNTER_FILE}")
    file(READ "${BUILD_COUNTER_FILE}" CAOS_BUILD_COUNT)
    string(STRIP "${CAOS_BUILD_COUNT}" CAOS_BUILD_COUNT)
    math(EXPR CAOS_BUILD_COUNT "${CAOS_BUILD_COUNT} + 1")
else()
    set(CAOS_BUILD_COUNT 1)
endif()

string(TOLOWER "${CAOS_DB_BACKEND}" CAOS_DB_BACKEND_LOWER)

# Try to find each supported Python version
foreach(PYTHON_VER IN LISTS SUPPORTED_PYTHON_VERSIONS)
    # Clear Python3 variables to ensure fresh detection
    unset(Python3_FOUND CACHE)
    unset(Python3_VERSION CACHE)
    unset(Python3_INCLUDE_DIRS CACHE)
    unset(Python3_LIBRARIES CACHE)
    unset(Python3_MODULE_EXTENSION_SUFFIX CACHE)

    find_package(Python3 ${PYTHON_VER} EXACT QUIET COMPONENTS Development Interpreter)
    if(Python3_FOUND)
        message(STATUS "Found Python ${Python3_VERSION} development package")

        # Get Python ABI suffix
        execute_process(
            COMMAND ${Python3_EXECUTABLE} -c "import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX') or sysconfig.get_config_var('SO'))"
            OUTPUT_VARIABLE PYTHON_MODULE_SUFFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(NOT PYTHON_MODULE_SUFFIX)
            # Fallback to default suffix
            if(Python3_VERSION VERSION_GREATER_EQUAL "3.8")
                set(PYTHON_MODULE_SUFFIX ".cpython-${CMAKE_MATCH_1}${CMAKE_MATCH_2}${CMAKE_SYSTEM_PROCESSOR}-linux-gnu.so")
            else()
                set(PYTHON_MODULE_SUFFIX ".so")
            endif()
        endif()

        message(STATUS "  Module Suffix: ${PYTHON_MODULE_SUFFIX}")

        string(REPLACE "." ";" PYTHON_VERSION_LIST ${Python3_VERSION})
        list(GET PYTHON_VERSION_LIST 0 PYTHON_VERSION_MAJOR)
        list(GET PYTHON_VERSION_LIST 1 PYTHON_VERSION_MINOR)
        set(PYTHON_VERSION_SHORT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")

        # Create unique target name for this Python version
        set(TARGET_NAME "${PROJECT_NAME}_python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}")
        list(APPEND PYTHON_TARGETS ${TARGET_NAME})
        list(APPEND PYTHON_BUILD_VERSIONS ${Python3_VERSION})

        # Configure file for this Python version
        configure_file(
            ${PROJECT_DIR}/src/include/extension_config.h.in
            ${CMAKE_BINARY_DIR}/extension_config.h
            @ONLY
        )

        # Create library target for this Python version
        add_library(${TARGET_NAME} MODULE
            ${PROJECT_DIR}/src/bindings.cpp
            ${PROJECT_DIR}/src/call_context.cpp
        )

        target_include_directories(${TARGET_NAME} PRIVATE
            ${CMAKE_BINARY_DIR}
            ${PROJECT_DIR}/src
            ${PROJECT_DIR}/src/include
            ${Python3_INCLUDE_DIRS}
        )

        # Generate correct module name with Python ABI suffix
        # Remove any existing suffix first
        string(REGEX REPLACE "\\.[^.]*$" "" MODULE_BASENAME "${PROJECT_NAME}")
        set_target_properties(${TARGET_NAME} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "${MODULE_BASENAME}"
            SUFFIX "${PYTHON_MODULE_SUFFIX}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/python_${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}"
        )

        target_link_libraries(${TARGET_NAME} PRIVATE libcaos ${Python3_LIBRARIES})
        target_compile_definitions(${TARGET_NAME} PRIVATE
            CAOS_BUILD_PYTHON_BINDING
            PYTHON_VERSION_MAJOR=${PYTHON_VERSION_MAJOR}
            PYTHON_VERSION_MINOR=${PYTHON_VERSION_MINOR}
        )

        # Create Python package directory for this version (in build directory)
        set(PYTHON_PACKAGE_DIR "${CMAKE_BINARY_DIR}/python_package_${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}/${PROJECT_NAME}")
        file(MAKE_DIRECTORY ${PYTHON_PACKAGE_DIR})
        list(APPEND PYTHON_PACKAGE_DIRS ${PYTHON_PACKAGE_DIR})

        # Create __init__.py for this Python version (in build directory)
        file(WRITE ${PYTHON_PACKAGE_DIR}/__init__.py [[
"""
${PROJECT_NAME} - CAOS Native Python Bindings
==============================================

Native extension for CAOS database operations.
Backend: ${CAOS_DB_BACKEND} (${CAOS_DB_BACKEND_LOWER})
Build: ${CAOS_BUILD_COUNT}
Python: ${Python3_VERSION}

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
    so_files = glob.glob(os.path.join(package_dir, "*.cpython-*.so"))

    if not so_files:
        raise ImportError(f"No native module found in {package_dir}")

    current_suffix = f"cpython-{sys.version_info.major}{sys.version_info.minor}"

    for so_file in so_files:
        if current_suffix in so_file:
            return so_file

    # If no exact match, use first available
    return so_files[0]

# Load the native module
module_path = _load_correct_module()
spec = importlib.util.spec_from_file_location('${PROJECT_NAME}', module_path)
module = importlib.util.module_from_spec(spec)
sys.modules[__name__] = module
spec.loader.exec_module(module)

# Expose all public functions from native module
for attr in dir(module):
    if not attr.startswith('_'):
        globals()[attr] = getattr(module, attr)

__version__ = '0.1.0'
__build__ = ${CAOS_BUILD_COUNT}
__backend__ = '${CAOS_DB_BACKEND_LOWER}'
__python_version__ = f"{sys.version_info.major}.{sys.version_info.minor}"

def get_build_info():
    """Return build information."""
    return {
        'module': '${PROJECT_NAME}',
        'version': __version__,
        'build': __build__,
        'backend': __backend__,
        'python_version': __python_version__
    }
]])

        # Create nested repository directory structure: repositories/${PROJECT_NAME}/python/
        set(PYTHON_REPO_DIR "${PROJECT_DIR}/dist/repositories/${PROJECT_NAME}/python")
        file(MAKE_DIRECTORY ${PYTHON_REPO_DIR})

        # POST_BUILD: copy to both structured directory AND direct location
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            # Copy .so file to Python package directory in build directory
            COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:${TARGET_NAME}>
                ${PYTHON_PACKAGE_DIR}/${PROJECT_NAME}${PYTHON_MODULE_SUFFIX}

            # ALSO copy to direct location in build root with simple name (for easy testing)
            COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:${TARGET_NAME}>
                ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.so

            COMMENT "Building ${PROJECT_NAME} for Python ${Python3_VERSION}"
            VERBATIM
        )

        # Create a separate target for copying to dist/ (only executed when explicitly requested)
        add_custom_target(${TARGET_NAME}_copy_to_dist
            # Create dist directory if needed
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_DIR}/dist/repositories/${PROJECT_NAME}/python
            # Copy .so file from build directory to dist directory
            COMMAND ${CMAKE_COMMAND} -E copy
                ${PYTHON_PACKAGE_DIR}/${PROJECT_NAME}${PYTHON_MODULE_SUFFIX}
                ${PROJECT_DIR}/dist/repositories/${PROJECT_NAME}/python/${PROJECT_NAME}${PYTHON_MODULE_SUFFIX}
            # Copy __init__.py from build directory to dist directory
            COMMAND ${CMAKE_COMMAND} -E copy
                ${PYTHON_PACKAGE_DIR}/__init__.py
                ${PROJECT_DIR}/dist/repositories/${PROJECT_NAME}/python/__init__.py
            DEPENDS ${TARGET_NAME}
            COMMENT "Copying ${PROJECT_NAME} Python files to dist directory for packaging"
        )

        # Installation for this Python version - Ubuntu/Debian uses dist-packages
        install(TARGETS ${TARGET_NAME}
                LIBRARY
                DESTINATION lib/python3/dist-packages/${PROJECT_NAME}
                COMPONENT python_${PYTHON_VERSION_MAJOR}_${PYTHON_VERSION_MINOR})

        install(FILES ${PYTHON_PACKAGE_DIR}/__init__.py
                DESTINATION lib/python3/dist-packages/${PROJECT_NAME}
                COMPONENT python_${PYTHON_VERSION_MAJOR}_${PYTHON_VERSION_MINOR})

        # Store installation path for package script
        set(PYTHON_INSTALL_DIR_${PYTHON_VERSION_MAJOR}_${PYTHON_VERSION_MINOR}
            "lib/python3/dist-packages/${PROJECT_NAME}"
            CACHE INTERNAL ""
        )

    endif()
endforeach()

# Check if any Python versions were found
if(NOT PYTHON_TARGETS)
    message(FATAL_ERROR "
No supported Python development packages found!

Required for building CAOS Python extension.
Supported Python versions: ${SUPPORTED_PYTHON_VERSIONS}

Installation commands for Ubuntu/Debian:
    sudo apt-get update
    sudo apt-get install python3-dev
    # For specific versions:
    sudo apt-get install python3.8-dev python3.9-dev python3.10-dev python3.11-dev python3.12-dev

After installation, re-run CMake with Python support.
")
endif()

message(STATUS "Building for Python versions: ${PYTHON_BUILD_VERSIONS}")
message(STATUS "Python targets created: ${PYTHON_TARGETS}")

# Create main library target to satisfy parent CMake's target_link_libraries
add_library(${PROJECT_NAME} OBJECT)

# Link the required libraries (matches line 13 in parent CMakeLists.txt)
target_link_libraries(${PROJECT_NAME} PRIVATE libcaos repoexception)

# Add dummy source file for OBJECT library
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/dummy_${PROJECT_NAME}.cpp
    "// Dummy source for ${PROJECT_NAME} library\n// This exists only to satisfy CMake dependency requirements\n")
target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/dummy_${PROJECT_NAME}.cpp)

message(STATUS "Created main library target '${PROJECT_NAME}' as OBJECT library")
message(STATUS "  - Linked with: libcaos, repoexception")

# Create a meta-target that depends on all Python version targets
add_custom_target(${PROJECT_NAME}_python_all DEPENDS ${PYTHON_TARGETS})

# Create a target that copies ALL Python versions to dist/
add_custom_target(do_copy_all_to_dist)
foreach(TARGET_NAME IN LISTS PYTHON_TARGETS)
    add_dependencies(do_copy_all_to_dist ${TARGET_NAME}_copy_to_dist)
endforeach()

# Ensure Python extensions are built when main target is built
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_python_all)

# Create aggregated Python package with all versions
set(AGGREGATE_PYTHON_PACKAGE_DIR "${CMAKE_BINARY_DIR}/python_package/${PROJECT_NAME}")
file(MAKE_DIRECTORY ${AGGREGATE_PYTHON_PACKAGE_DIR})

# Create aggregated __init__.py that works for all Python versions
file(WRITE ${AGGREGATE_PYTHON_PACKAGE_DIR}/__init__.py [[
"""
${PROJECT_NAME} - CAOS Native Python Bindings
==============================================

Native extension for CAOS database operations.
Backend: ${CAOS_DB_BACKEND} (${CAOS_DB_BACKEND_LOWER})
Build: ${CAOS_BUILD_COUNT}

This package contains native extensions for multiple Python versions.
The correct version will be loaded automatically based on your Python interpreter.

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
    so_files = glob.glob(os.path.join(package_dir, "*.cpython-*.so"))

    if not so_files:
        raise ImportError(f"No native module found in {package_dir}")

    current_suffix = f"cpython-{sys.version_info.major}{sys.version_info.minor}"

    for so_file in so_files:
        if current_suffix in so_file:
            return so_file

    # If no exact match, use first available
    return so_files[0]

# Load the native module
module_path = _load_correct_module()
spec = importlib.util.spec_from_file_location('${PROJECT_NAME}', module_path)
module = importlib.util.module_from_spec(spec)
sys.modules[__name__] = module
spec.loader.exec_module(module)

# Expose all public functions from native module
for attr in dir(module):
    if not attr.startswith('_'):
        globals()[attr] = getattr(module, attr)

__version__ = '0.1.0'
__build__ = ${CAOS_BUILD_COUNT}
__backend__ = '${CAOS_DB_BACKEND_LOWER}'
__python_version__ = f"{sys.version_info.major}.{sys.version_info.minor}"

def get_build_info():
    """Return build information."""
    return {
        'module': '${PROJECT_NAME}',
        'version': __version__,
        'build': __build__,
        'backend': __backend__,
        'python_version': __python_version__
    }
]])

# Copy all version-specific modules to aggregate package
foreach(PYTHON_PACKAGE_DIR IN LISTS PYTHON_PACKAGE_DIRS)
    add_custom_command(TARGET ${PROJECT_NAME}_python_all POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PYTHON_PACKAGE_DIR}
            ${AGGREGATE_PYTHON_PACKAGE_DIR}
        COMMENT "Aggregating Python modules for all versions"
    )
endforeach()

# Create aggregated setup.py
file(WRITE ${CMAKE_BINARY_DIR}/python_package/setup.py [[
from setuptools import setup, find_packages
import sys
import os

# Get version from __init__.py
with open(os.path.join('${PROJECT_NAME}', '__init__.py'), 'r') as f:
    for line in f:
        if line.startswith('__version__'):
            version = line.split('=')[1].strip().strip('\"\'')
            break

setup(
    name='${PROJECT_NAME}',
    version=version,
    description='CAOS Native Python Bindings for ${PROJECT_NAME}',
    author='CAOS Development Team',
    packages=find_packages(),
    package_data={
        '${PROJECT_NAME}': ['*.so'],
    },
    classifiers=[
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
    ],
    python_requires='>=3.8',
)
]])

# Installation messages for all Python versions
install(CODE "
    message(STATUS \"\")
    message(STATUS \"Python extension '${PROJECT_NAME}' built for versions: ${PYTHON_BUILD_VERSIONS}\")
    message(STATUS \"Build: ${CAOS_BUILD_COUNT}, Backend: ${CAOS_DB_BACKEND}\")
    message(STATUS \"\")
    message(STATUS \"Installed in: lib/python3/dist-packages/${PROJECT_NAME}/\")
    message(STATUS \"\")
    message(STATUS \"To test the installation:\")
    message(STATUS \"  python3 -c \\\"import ${PROJECT_NAME}; print(${PROJECT_NAME}.get_build_info())\\\"\")
    message(STATUS \"\")"
    COMPONENT python
)

# Package targets
if(CMAKE_BUILD_TYPE STREQUAL "release")
    add_custom_target(make_package_deb
        COMMAND ${PROJECT_DIR}/scripts/create_package_deb.sh ${CAOS_DB_BACKEND} ${PROJECT_NAME}
        DEPENDS libcaos do_copy_all_to_dist
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Building DEB package for Python extension '${PROJECT_NAME}' with ${CAOS_DB_BACKEND} backend"
    )

    add_custom_target(make_distribution_tarball
        COMMAND ${PROJECT_DIR}/scripts/create_distribution_tarball.sh ${CAOS_DB_BACKEND} ${PROJECT_NAME}
        DEPENDS make_package_deb
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating distribution tarball for Python extension '${PROJECT_NAME}' with ${CAOS_DB_BACKEND} backend"
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
