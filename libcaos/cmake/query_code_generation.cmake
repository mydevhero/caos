# =============================================================================
# Query Interface Generation
#
# Generates C++ boilerplate code for query interfaces from YAML definitions.
# Supports both core CAOSDBA queries and optional custom queries.
# =============================================================================

# Core CAOSDBA queries (always required)
set(CAOSDBA_QUERIES_DIR "${CMAKE_SOURCE_DIR}/CAOSDBA/queries/CAOSDBA")
set(CAOSDBA_SCHEMA_DIR "${CMAKE_SOURCE_DIR}/CAOSDBA/schemas/CAOSDBA")

# Check for core queries (required)
if(NOT EXISTS "${CAOSDBA_QUERIES_DIR}/queries.yaml")
  message(FATAL_ERROR
    "Core CAOSDBA query definitions not found! "
    "Expected: ${CAOSDBA_QUERIES_DIR}/queries.yaml"
  )
endif()

if(NOT EXISTS "${CAOSDBA_SCHEMA_DIR}/queries.json")
  message(FATAL_ERROR
    "Core CAOSDBA query schema not found! "
    "Expected: ${CAOSDBA_SCHEMA_DIR}/queries.json"
  )
endif()

# Check for optional custom queries
set(CUSTOM_QUERIES_DIR "${CMAKE_SOURCE_DIR}/queries/custom")
set(CUSTOM_SCHEMA_DIR "${CMAKE_SOURCE_DIR}/schemas/custom")

set(CUSTOM_QUERIES_FOUND FALSE)
set(CUSTOM_SCHEMA_FOUND FALSE)

if(EXISTS "${CUSTOM_QUERIES_DIR}/queries.yaml")
  set(CUSTOM_QUERIES_FOUND TRUE)
  message(STATUS "Found custom queries at: ${CUSTOM_QUERIES_DIR}/queries.yaml")
else()
  message(STATUS "No custom queries found (optional): ${CUSTOM_QUERIES_DIR}/queries.yaml")
endif()

if(EXISTS "${CUSTOM_SCHEMA_DIR}/queries.json")
  set(CUSTOM_SCHEMA_FOUND TRUE)
  message(STATUS "Found custom query schema at: ${CUSTOM_SCHEMA_DIR}/queries.json")
elseif(CUSTOM_QUERIES_FOUND)
  message(WARNING
    "Custom queries found but schema is missing! "
    "Expected: ${CUSTOM_SCHEMA_DIR}/queries.json. "
    "Custom queries will be ignored."
  )
  set(CUSTOM_QUERIES_FOUND FALSE)
endif()

# Set flags for enabling example/template queries
if(CAOS_BUILD_EXAMPLES AND (CAOS_BUILD_PHP_BINDING OR CAOS_BUILD_NODE_BINDING OR CAOS_BUILD_PYTHON_BINDING OR CAOS_USE_CROWCPP))
  set(CAOS_EXAMPLE_QUERY "--caos-example-query")
endif()

set(CAOS_TEMPLATE_CODE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CAOS_TEMPLATE_CODE_FILE "${CAOS_TEMPLATE_CODE_PATH}/type_code.cmake")
if(EXISTS ${CAOS_TEMPLATE_CODE_FILE})
  include(${CAOS_TEMPLATE_CODE_FILE})
endif()

if(CAOS_TEMPLATE_CODE OR CAOS_CROWCPP_CODE)
  set(CAOS_TEMPLATE_QUERY "--caos-template-query")
endif()

# Find Python interpreter
find_package(Python3 REQUIRED)

# Set paths for core and custom queries
set(CAOSDBA_QUERY_DEFINITIONS "${CAOSDBA_QUERIES_DIR}/queries.yaml")
set(CAOSDBA_QUERY_SCHEMA "${CAOSDBA_SCHEMA_DIR}/queries.json")

if(CUSTOM_QUERIES_FOUND)
  set(CUSTOM_QUERY_DEFINITIONS "${CUSTOM_QUERIES_DIR}/queries.yaml")
  set(CUSTOM_QUERY_SCHEMA "${CUSTOM_SCHEMA_DIR}/queries.json")
endif()

# Set generator script and output directory
set(QUERY_GENERATOR_SCRIPT "${CMAKE_SOURCE_DIR}/CAOSDBA/bin/generate_queries.py")
set(GENERATED_QUERIES_DIR "${CMAKE_BINARY_DIR}/generated_queries")

# Validate required files
if(NOT EXISTS "${QUERY_GENERATOR_SCRIPT}")
  message(FATAL_ERROR
    "Query generator script not found! "
    "Expected: ${QUERY_GENERATOR_SCRIPT}"
  )
endif()

# Create output directory
file(MAKE_DIRECTORY "${GENERATED_QUERIES_DIR}")

# Add dependency tracking for all definition and schema files
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${QUERY_GENERATOR_SCRIPT}
  ${CAOSDBA_QUERY_DEFINITIONS}
  ${CAOSDBA_QUERY_SCHEMA}
)

if(CUSTOM_QUERIES_FOUND)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${CUSTOM_QUERY_DEFINITIONS}
    ${CUSTOM_QUERY_SCHEMA}
  )
endif()

message(STATUS "Generating query interfaces...")
message(STATUS "  Core queries: ${CAOSDBA_QUERY_DEFINITIONS}")

if(CUSTOM_QUERIES_FOUND)
  message(STATUS "  Custom queries: ${CUSTOM_QUERY_DEFINITIONS}")
endif()

# Execute query generator script
# Execute query generator script
if(CUSTOM_QUERIES_FOUND)
  execute_process(
    COMMAND ${Python3_EXECUTABLE}
      ${QUERY_GENERATOR_SCRIPT}
      --core-definitions ${CAOSDBA_QUERY_DEFINITIONS}
      --core-schema ${CAOSDBA_QUERY_SCHEMA}
      --custom-definitions ${CUSTOM_QUERY_DEFINITIONS}
      --custom-schema ${CUSTOM_QUERY_SCHEMA}
      --output-dir ${GENERATED_QUERIES_DIR}
      ${CAOS_EXAMPLE_QUERY}
      ${CAOS_TEMPLATE_QUERY}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE QUERY_GEN_RESULT
    OUTPUT_VARIABLE QUERY_GEN_OUTPUT
    ERROR_VARIABLE QUERY_GEN_ERROR
  )
else()
  execute_process(
    COMMAND ${Python3_EXECUTABLE}
      ${QUERY_GENERATOR_SCRIPT}
      --core-definitions ${CAOSDBA_QUERY_DEFINITIONS}
      --core-schema ${CAOSDBA_QUERY_SCHEMA}
      --output-dir ${GENERATED_QUERIES_DIR}
      ${CAOS_EXAMPLE_QUERY}
      ${CAOS_TEMPLATE_QUERY}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    RESULT_VARIABLE QUERY_GEN_RESULT
    OUTPUT_VARIABLE QUERY_GEN_OUTPUT
    ERROR_VARIABLE QUERY_GEN_ERROR
  )
endif()

# Debug output
if(QUERY_GEN_OUTPUT)
  message(STATUS "Query generator output: ${QUERY_GEN_OUTPUT}")
endif()

if(QUERY_GEN_ERROR)
  message(WARNING "Query generator errors: ${QUERY_GEN_ERROR}")
endif()

if(NOT QUERY_GEN_RESULT EQUAL 0)
  message(FATAL_ERROR
    "Query generation failed!\n"
    "Result code: ${QUERY_GEN_RESULT}\n"
    "Output: ${QUERY_GEN_OUTPUT}\n"
    "Error: ${QUERY_GEN_ERROR}"
  )
endif()

message(STATUS "Query generation completed successfully")

# Set generated files
set(GENERATED_QUERY_DEFINITION "${GENERATED_QUERIES_DIR}/Query_Definition.hpp")
set(GENERATED_QUERY_OVERRIDE "${GENERATED_QUERIES_DIR}/Query_Override.hpp")
set(GENERATED_CACHE_FORWARDING "${GENERATED_QUERIES_DIR}/Cache_Query_Forwarding.hpp")
set(GENERATED_DATABASE_FORWARDING "${GENERATED_QUERIES_DIR}/Database_Query_Forwarding.hpp")
set(GENERATED_AUTH_CONFIG "${GENERATED_QUERIES_DIR}/AuthConfig.hpp")
set(GENERATED_QUERY_CONFIG "${GENERATED_QUERIES_DIR}/Query_Config.cmake")

# Include generated CMake configuration
include(${GENERATED_QUERY_CONFIG})

# Add generated directory to include path
target_include_directories(${PROJECT_NAME} PRIVATE
  ${GENERATED_QUERIES_DIR}
)
