if(NOT EXISTS ${CMAKE_SOURCE_DIR}/dist)
  file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/dist)
endif()

# Select CAOS_ENV ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "release")
endif()
# Select CAOS_ENV ----------------------------------------------------------------------------------

option(CAOS_USE_CACHE               "CAOS using Cache"                ON) # Means only Redis so far
option(CAOS_BUILD_EXAMPLES          "CAOS build examples"             OFF)
option(CAOS_USE_CROWCPP             "CAOS use Crow"                   OFF)
option(CAOS_BUILD_BINDINGS          "CAOS build bindings extension"   OFF)

# BINDINGS +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
if(CAOS_BUILD_BINDINGS)
  option(CAOS_BUILD_PHP_BINDING     "CAOS build php extension"        ON)
  option(CAOS_BUILD_NODE_BINDING    "CAOS build node.js extension"    OFF) # Not ready yet
  option(CAOS_BUILD_PYTHON_BINDING  "CAOS build python extension"     OFF) # Not ready yet
endif()
# BINDINGS -----------------------------------------------------------------------------------------

# Select CAOS_DB_BACKEND +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
set(VALID_DB_BACKENDS "POSTGRESQL" "MYSQL" "MARIADB")
set(CAOS_DB_BACKEND_PATH "${CMAKE_SOURCE_DIR}/cmake") # -------------
set(CAOS_DB_BACKEND_FILE "${CAOS_DB_BACKEND_PATH}/db_backend.cmake")
if(EXISTS ${CAOS_DB_BACKEND_FILE})
  include(${CAOS_DB_BACKEND_FILE})

  set(CAOS_DB_BACKEND "${DB_BACKEND}" CACHE STRING "Database backend, got default from cmake/db_backend.cmake")
  message(STATUS "Using CAOS_DB_BACKEND from db_backend.cmake: ${CAOS_DB_BACKEND}")

elseif(DEFINED CAOS_DB_BACKEND)
  if(NOT CAOS_DB_BACKEND IN_LIST VALID_DB_BACKENDS)
    message(FATAL_ERROR
      "Invalid database backend '${CAOS_DB_BACKEND}'\n"
      "Valid options are (uppercase only): ${VALID_DB_BACKENDS}\n"
      "Example: -DCAOS_DB_BACKEND=POSTGRESQL"
    )
  endif()

  message(STATUS "Using CAOS_DB_BACKEND from command line: ${CAOS_DB_BACKEND}")

  # Save
  file(MAKE_DIRECTORY "${CAOS_DB_BACKEND_PATH}")
  file(WRITE ${CAOS_DB_BACKEND_FILE}
    "# Database backend configuration\n"
    "set(DB_BACKEND ${CAOS_DB_BACKEND})\n"
  )
  message(STATUS "Saved database backend configuration: ${CAOS_DB_BACKEND}")

else()
  message(FATAL_ERROR "CAOS_DB_BACKEND not defined. Please specify with -DCAOS_DB_BACKEND=<value>\n"
          "Available options (uppercase only): ${VALID_DB_BACKENDS}")
endif()

set_property(CACHE CAOS_DB_BACKEND PROPERTY STRINGS
  "POSTGRESQL"
  "MYSQL"
  "MARIADB"
)
# Select CAOS_DB_BACKEND ---------------------------------------------------------------------------

# Select CAOS_PROJECT_TYPE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
set(CAOS_PROJECT_TYPE_PATH "${CMAKE_SOURCE_DIR}/cmake") # -------------
set(CAOS_PROJECT_TYPE_FILE "${CAOS_PROJECT_TYPE_PATH}/project_type.cmake")
if(EXISTS ${CAOS_PROJECT_TYPE_FILE})
  include(${CAOS_PROJECT_TYPE_FILE})

  set(CAOS_PROJECT_TYPE "${PROJECT_TYPE}" CACHE STRING "Project Type, got default from cmake/project_type.cmake")
  message(STATUS "Using CAOS_PROJECT_TYPE from project_type.cmake: ${CAOS_PROJECT_TYPE}")

elseif(DEFINED CAOS_PROJECT_TYPE)
  message(STATUS "Using CAOS_PROJECT_TYPE from command line: ${CAOS_PROJECT_TYPE}")

  # Save
  file(MAKE_DIRECTORY "${CAOS_PROJECT_TYPE_PATH}")
  file(WRITE ${CAOS_PROJECT_TYPE_FILE}
    "# Project Type configuration\n"
    "set(PROJECT_TYPE ${CAOS_PROJECT_TYPE})\n"
  )
  message(STATUS "Saved project type configuration: ${CAOS_PROJECT_TYPE}")

else()
  message(FATAL_ERROR "CAOS_PROJECT_TYPE not defined. Please specify with -DCAOS_PROJECT_TYPE=<value>\n"
            "Available options: STANDALONE, CROWCPP, BINDING")
endif()

set_property(CACHE CAOS_PROJECT_TYPE PROPERTY STRINGS
  "STANDALONE"
  "CROWCPP"
  "BINDING"
)
# Select CAOS_PROJECT_TYPE -------------------------------------------------------------------------

# Select CAOS_BINDING_LANGUAGE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
if(DEFINED CAOS_PROJECT_TYPE AND CAOS_PROJECT_TYPE STREQUAL "BINDING")
  # Definisci i linguaggi validi (solo maiuscoli)
  set(VALID_BINDING_LANGUAGES "PHP" "NODEJS" "PYTHON")
  set(CAOS_BINDING_LANGUAGE_PATH "${CMAKE_SOURCE_DIR}/cmake") # -------------
  set(CAOS_BINDING_LANGUAGE_FILE "${CAOS_BINDING_LANGUAGE_PATH}/binding_language.cmake")
  if(EXISTS ${CAOS_BINDING_LANGUAGE_FILE})
    include(${CAOS_BINDING_LANGUAGE_FILE})

    set(CAOS_BINDING_LANGUAGE "${BINDING_LANGUAGE}" CACHE STRING "Binding language, got default from cmake/binding_language.cmake")
    message(STATUS "Using CAOS_BINDING_LANGUAGE from binding_language.cmake: ${CAOS_BINDING_LANGUAGE}")

  elseif(DEFINED CAOS_BINDING_LANGUAGE)
    if(NOT CAOS_BINDING_LANGUAGE IN_LIST VALID_BINDING_LANGUAGES)
      message(FATAL_ERROR
        "Invalid binding language '${CAOS_BINDING_LANGUAGE}'\n"
        "Valid options are (uppercase only): ${VALID_BINDING_LANGUAGES}\n"
        "Example: -DCAOS_BINDING_LANGUAGE=NODEJS"
      )
    endif()

    message(STATUS "Using CAOS_BINDING_LANGUAGE from command line: ${CAOS_BINDING_LANGUAGE}")

    # Save
    file(MAKE_DIRECTORY ${CAOS_BINDING_LANGUAGE_PATH})
    file(WRITE ${CAOS_BINDING_LANGUAGE_FILE}
      "# Binding language configuration\n"
      "set(BINDING_LANGUAGE ${CAOS_BINDING_LANGUAGE})\n"
    )
    message(STATUS "Saved project type configuration: ${CAOS_BINDING_LANGUAGE}")

  else()
    message(FATAL_ERROR "CAOS_BINDING_LANGUAGE not defined. Please specify with -DCAOS_BINDING_LANGUAGE=<value>\n"
            "Available options (uppercase only): ${VALID_BINDING_LANGUAGES}")
  endif()

  set_property(CACHE CAOS_BINDING_LANGUAGE PROPERTY STRINGS
    "PHP"
    "NODEJS"
    "PYTHON"
  )
endif()
# Select CAOS_BINDING_LANGUAGE ---------------------------------------------------------------------

# Select CAOS_CROWCPP_TYPE +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
if(DEFINED CAOS_PROJECT_TYPE AND CAOS_PROJECT_TYPE STREQUAL "CROWCPP")
  set(CAOS_CROWCPP_TYPE_PATH "${CMAKE_SOURCE_DIR}/cmake") # -------------
  set(CAOS_CROWCPP_TYPE_FILE "${CAOS_CROWCPP_TYPE_PATH}/crowcpp_type.cmake")

  if(EXISTS ${CAOS_CROWCPP_TYPE_FILE})
    include(${CAOS_CROWCPP_TYPE_FILE})
    set(CAOS_CROWCPP_TYPE "${CROWCPP_TYPE}" CACHE STRING "CROWCPP type, got default from cmake/crowcpp_type.cmake")
    message(STATUS "Using CAOS_CROWCPP_TYPE from crowcpp_type.cmake: ${CAOS_CROWCPP_TYPE}")

  elseif(DEFINED CAOS_CROWCPP_TYPE)
    message(STATUS "Using CAOS_CROWCPP_TYPE from command line: ${CAOS_CROWCPP_TYPE}")

    # Save
    file(MAKE_DIRECTORY ${CAOS_CROWCPP_TYPE_PATH})
    file(WRITE ${CAOS_CROWCPP_TYPE_FILE}
      "# CROWCPP type configuration\n"
      "set(CROWCPP_TYPE ${CAOS_CROWCPP_TYPE})\n"
    )
    message(STATUS "Saved CROWCPP type configuration: ${CAOS_CROWCPP_TYPE}")

  else()
    message(FATAL_ERROR "CAOS_CROWCPP_TYPE not defined. Please specify with -DCAOS_CROWCPP_TYPE=<value>\n"
            "Available options: ENDPOINT, MIDDLEWARE")
  endif()

  set_property(CACHE CAOS_CROWCPP_TYPE PROPERTY STRINGS
    "ENDPOINT"
    "MIDDLEWARE"
  )
endif()
# Select CAOS_CROWCPP_TYPE -------------------------------------------------------------------------

# Initialize project structure ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Execute setup script to initialize project files from templates
message(STATUS "Initializing project structure...")

execute_process(
  COMMAND ${CMAKE_SOURCE_DIR}/CAOSDBA/bin/caosdba.sh --init ${PROJECT_NAME}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/CAOSDBA
  OUTPUT_VARIABLE SETUP_OUTPUT
  ERROR_VARIABLE SETUP_ERROR
  RESULT_VARIABLE SETUP_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_STRIP_TRAILING_WHITESPACE
)

set(CAOS_TEMPLATE_CODE_PATH "${CMAKE_SOURCE_DIR}/cmake") # -------------
set(CAOS_TEMPLATE_CODE_FILE "${CAOS_TEMPLATE_CODE_PATH}/type_code.cmake")
if(EXISTS ${CAOS_TEMPLATE_CODE_FILE})
  include(${CAOS_TEMPLATE_CODE_FILE})
endif()

# Display setup output
if(SETUP_OUTPUT)
  message(STATUS "${SETUP_OUTPUT}")
endif()

# Check for errors
if(NOT SETUP_RESULT EQUAL 0)
  if(SETUP_ERROR)
    message(WARNING "Setup script encountered an issue:\n${SETUP_ERROR}")
  else()
    message(WARNING "Setup script failed with return code: ${SETUP_RESULT}")
  endif()
  message(WARNING "Project initialization may be incomplete. Please check the setup script.")
endif()
# Initialize project structure ---------------------------------------------------------------------





# Select CAOS_USE_CACHE ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
if(CAOS_USE_CACHE)
  set(CAOS_CACHE_BACKEND "REDIS" CACHE STRING "Cache backend to use for CAOS." )
  set_property(CACHE CAOS_CACHE_BACKEND PROPERTY STRINGS "REDIS")
endif()
# Select CAOS_USE_CACHE ----------------------------------------------------------------------------

if(CAOS_BUILD_EXAMPLES)
  # PHP
  if(CAOS_BUILD_PHP_BINDING)
    option(CAOS_BUILD_PHP_BINDING_EXAMPLE  "CAOS build php example"               OFF)
    option(CAOS_BUILD_PHP_PACKAGE_DEB      "CAOS build php package Debian/Ubuntu" OFF)
  endif()

  # NODE
  if(CAOS_BUILD_NODE_BINDING)
    option(CAOS_BUILD_NODE_BINDING_EXAMPLE  "CAOS build node example"      OFF)
    # set(CAOS_BUILD_NODE_PACKAGE_DEB        "CAOS build node package Debian/Ubuntu" ON)
  endif()

  # PYTHON
  if(CAOS_BUILD_PYTHON_BINDING)
    option(CAOS_BUILD_PYTHON_BINDING_EXAMPLE  "CAOS build python example"  OFF)
    # set(CAOS_BUILD_PYTHON_PACKAGE_DEB        "CAOS build python package Debian/Ubuntu" ON)
  endif()

  option(CAOS_BUILD_STRESS_EXAMPLE          "CAOS build stress example"  OFF)
endif()

# CROWCPP
if(CAOS_USE_CROWCPP)
  if(CAOS_BUILD_EXAMPLES)
    option(CAOS_BUILD_CROWCPP_EXAMPLE "CAOS build CrowCpp example"  OFF)
    # set(CAOS_BUILD_CROWCPP_PACKAGE_DEB        "CAOS build crowcpp package Debian" ON)
  endif()
endif()

# Add libcaos ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
add_subdirectory(CAOSDBA/libcaos)
add_library(repoexception INTERFACE)
target_include_directories(repoexception INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libcaos/Middleware/Repository)
target_link_libraries(repoexception INTERFACE libcaos)
# Add libcaos --------------------------------------------------------------------------------------

# BINDINGS +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
if(CAOS_BUILD_BINDINGS)
  add_subdirectory(bindings)
endif()
# BINDINGS -----------------------------------------------------------------------------------------

# Option CAOS_BUILD_EXAMPLES +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
if(CAOS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
# Option CAOS_BUILD_EXAMPLES -----------------------------------------------------------------------

# Get query boilerplate code
include(${CMAKE_BINARY_DIR}/generated_queries/Query_Config.cmake)
