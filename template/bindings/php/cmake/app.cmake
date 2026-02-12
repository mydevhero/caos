message(STATUS "Building CAOS as PHP extension")

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_DIR ${CMAKE_SOURCE_DIR})

find_program(PHP_CONFIG php-config)

if(PHP_CONFIG)
  execute_process(COMMAND ${PHP_CONFIG} --includes
    OUTPUT_VARIABLE PHP_INCLUDE_FLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(COMMAND ${PHP_CONFIG} --libs
    OUTPUT_VARIABLE PHP_LIBRARIES
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(COMMAND ${PHP_CONFIG} --version
    OUTPUT_VARIABLE PHP_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  execute_process(COMMAND ${PHP_CONFIG} --extension-dir
    OUTPUT_VARIABLE PHP_EXTENSION_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  message(STATUS "PHP development package found")
  message(STATUS "  Version: ${PHP_VERSION}")
  message(STATUS "  Extension Dir: ${PHP_EXTENSION_DIR}")

  if(PHP_VERSION VERSION_LESS "8.0")
    message(FATAL_ERROR "PHP version ${PHP_VERSION} is too old. Required: 8.0+")
  endif()

  separate_arguments(PHP_INCLUDE_FLAGS)

  # Add call_context.cpp to source files
  add_library(${PROJECT_NAME} MODULE
    ${PROJECT_DIR}/src/bindings.cpp
    ${PROJECT_DIR}/src/call_context.cpp
  )

  configure_file(
    ${PROJECT_DIR}/src/include/extension_config.h.in
    ${CMAKE_BINARY_DIR}/extension_config.h
    @ONLY
  )

  target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_BINARY_DIR}
    ${PROJECT_DIR}/src
    ${PROJECT_DIR}/src/include
  )

  set_target_properties(${PROJECT_NAME} PROPERTIES
    PREFIX ""
    OUTPUT_NAME "${PROJECT_NAME}"
    SUFFIX ".so"
  )

  # Add libcaos to link libraries
  target_link_libraries(${PROJECT_NAME} PRIVATE libcaos ${PHP_LIBRARIES})
  target_compile_options(${PROJECT_NAME} PRIVATE ${PHP_INCLUDE_FLAGS})
  target_compile_definitions(${PROJECT_NAME} PUBLIC CAOS_BUILD_PHP_BINDING)

  string(REGEX REPLACE "^([0-9]+\\.[0-9]+).*" "\\1" PHP_VERSION_SHORT "${PHP_VERSION}")
  message(STATUS "PHP Version Short: ${PHP_VERSION_SHORT}")

  set(PHP_MODS_AVAILABLE_DIR "/etc/php/${PHP_VERSION_SHORT}/mods-available")
  set(PHP_CLI_CONF_D_DIR "/etc/php/${PHP_VERSION_SHORT}/cli/conf.d")
  set(PHP_FPM_CONF_D_DIR "/etc/php/${PHP_VERSION_SHORT}/fpm/conf.d")
  set(PHP_APACHE_CONF_D_DIR "/etc/php/${PHP_VERSION_SHORT}/apache2/conf.d")

  set(CAOS_INI_CONTENT "extension=${PROJECT_NAME}.so\n")
  file(WRITE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.ini "${CAOS_INI_CONTENT}")

  # Unified build counter logic (same as Python)
  set(BUILD_COUNTER_FILE "${CMAKE_BINARY_DIR}/build_counter.txt")
  if(EXISTS "${BUILD_COUNTER_FILE}")
    file(READ "${BUILD_COUNTER_FILE}" CAOS_BUILD_COUNT)
    string(STRIP "${CAOS_BUILD_COUNT}" CAOS_BUILD_COUNT)
    math(EXPR CAOS_BUILD_COUNT "${CAOS_BUILD_COUNT} + 1")
  else()
    set(CAOS_BUILD_COUNT 1)
  endif()

  string(TOLOWER "${CAOS_DB_BACKEND}" CAOS_DB_BACKEND_LOWER)

  # Organized package directory
  set(PHP_PACKAGE_DIR "${CMAKE_BINARY_DIR}/php_package")
  file(MAKE_DIRECTORY ${PHP_PACKAGE_DIR})

  # Create nested repository directory structure: repositories/${PROJECT_NAME}/php/
  set(PHP_REPO_DIR "${PROJECT_DIR}/dist/repositories/${PROJECT_NAME}/php")
  file(MAKE_DIRECTORY ${PHP_REPO_DIR})

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # Copy to organized package directory
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${PHP_PACKAGE_DIR}/${PROJECT_NAME}.so

    # Copy to project-specific nested repository directory
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${PHP_REPO_DIR}/${PROJECT_NAME}-${CAOS_DB_BACKEND_LOWER}-${CAOS_BUILD_COUNT}.so

    # Copy ini file to repository directory
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.ini
        ${PHP_REPO_DIR}/${PROJECT_NAME}-${CAOS_DB_BACKEND_LOWER}-${CAOS_BUILD_COUNT}.ini

    COMMENT "Building ${PROJECT_NAME} for PHP ${PHP_VERSION}"
    VERBATIM
  )

  # Add copy to repository target for package dependency
  add_custom_target(do_copy_to_dist
    DEPENDS ${PROJECT_NAME}
    COMMENT "Target for copying PHP extension to repository directory"
  )

  install(TARGETS ${PROJECT_NAME}
          LIBRARY
          DESTINATION ${PHP_EXTENSION_DIR}
          COMPONENT php)

  install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.ini
          DESTINATION ${PHP_MODS_AVAILABLE_DIR}
          RENAME ${PROJECT_NAME}.ini
          COMPONENT php)

  install(CODE "message(STATUS \"\")
                message(STATUS \"PHP extension installed to: ${PHP_EXTENSION_DIR}/${PROJECT_NAME}.so\")
                message(STATUS \"PHP ini file installed to: ${PHP_MODS_AVAILABLE_DIR}/${PROJECT_NAME}.ini\")
                message(STATUS \"\")
                message(STATUS \"To enable the extension, run:\")
                message(STATUS \"  sudo phpenmod -v ${PHP_VERSION_SHORT} ${PROJECT_NAME}\")
                message(STATUS \"\")
                message(STATUS \"Or enable for specific SAPI:\")
                message(STATUS \"  sudo phpenmod -v ${PHP_VERSION_SHORT} -s cli ${PROJECT_NAME}\")
                message(STATUS \"  sudo phpenmod -v ${PHP_VERSION_SHORT} -s fpm ${PROJECT_NAME}\")
                message(STATUS \"  sudo phpenmod -v ${PHP_VERSION_SHORT} -s apache2 ${PROJECT_NAME}\")
                message(STATUS \"\")"
          COMPONENT php)

  message(STATUS "PHP extension will be installed to: ${PHP_EXTENSION_DIR}/${PROJECT_NAME}.so")
  message(STATUS "PHP ini file will be installed to: ${PHP_MODS_AVAILABLE_DIR}/${PROJECT_NAME}.ini")

  if(CMAKE_BUILD_TYPE STREQUAL "release")
      add_custom_target(make_package_deb
          COMMAND ${PROJECT_DIR}/scripts/create_package_deb.sh ${CAOS_DB_BACKEND} ${PROJECT_NAME}
          DEPENDS libcaos do_copy_to_dist
          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
          COMMENT "Building DEB package for PHP extension '${PROJECT_NAME}' with ${CAOS_DB_BACKEND} backend"
      )

      add_custom_target(make_distribution_tarball
          COMMAND ${PROJECT_DIR}/scripts/create_distribution_tarball.sh ${CAOS_DB_BACKEND} ${PROJECT_NAME}
          DEPENDS make_package_deb
          WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
          COMMENT "Creating distribution tarball for PHP extension '${PROJECT_NAME}' with ${CAOS_DB_BACKEND} backend"
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
else()
  message(FATAL_ERROR "
PHP development packages not found!

Required for building CAOS PHP extension.

Installation commands:
Ubuntu/Debian: sudo apt-get install php-dev
")
endif()
