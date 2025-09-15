cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(libcaos VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Og -rdynamic -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -Og -rdynamic -fno-omit-frame-pointer")

# Opzione per attivare/disattivare Clang-Tidy
option(ENABLE_CLANG_TIDY "Enable static analysis with Clang-Tidy" OFF)
if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE "clang-tidy")

  if(CLANG_TIDY_EXE)
    # Configura i check dal file .clang-tidy
    set(
      CMAKE_CXX_CLANG_TIDY
      "${CLANG_TIDY_EXE}"
      "--config-file=.clang-tidy"
      "--extra-arg-before=--driver-mode=g++"
    )

    message(STATUS "Clang-Tidy attivato: ${CLANG_TIDY_EXE}")
  else()
    message(WARNING "Clang-Tidy non trovato. Installalo con: sudo apt-get install clang-tidy")
  endif()
endif()

include(cmake/CPM.cmake)
include_directories("${CMAKE_BINARY_DIR}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/config.hpp)

add_custom_target(increment_build ALL
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/increment_build_counter.cmake
  BYPRODUCTS "${CMAKE_BINARY_DIR}/build_counter.txt"
  COMMENT "Incrementing build counter"
)

# Aggiungi sorgenti
add_library(${PROJECT_NAME} STATIC ${PROJECT_NAME}.cpp
  Middleware/Repository/IRepository.hpp
  Middleware/Repository/Database/Database.cpp
  Middleware/Repository/Database/Database.hpp
  Middleware/Repository/Cache/Cache.hpp
  Middleware/Repository/Database/PostgreSQL/PostgreSQL.cpp
  Middleware/Repository/Database/PostgreSQL/PostgreSQL.hpp
  Middleware/Middleware.hpp
  terminal_options.cpp
  include/terminal_options.hpp
  environment.cpp
  include/environment.hpp
  Middleware/Repository/Exception.hpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Sanitizer abilitati per build Debug")
  target_compile_options(${PROJECT_NAME} PRIVATE
    -fsanitize=address
    -fsanitize=undefined
    -fsanitize=leak
    -fno-omit-frame-pointer
  )
endif()

include(FetchContent)

# apt install libpq-dev libcxxopts-dev
find_package(PostgreSQL REQUIRED)
find_package(cxxopts REQUIRED)
# find_package(CURL REQUIRED)       # apt install libcurl4-openssl-dev

# Fetch libpqxx
FetchContent_Declare(
    libpqxx
    GIT_REPOSITORY https://github.com/jtv/libpqxx.git
    GIT_TAG 7.10.1
)

# Recupera hiredis
FetchContent_Declare(
  hiredis
  GIT_REPOSITORY https://github.com/redis/hiredis.git
  GIT_TAG v1.1.0 # Sostituisci con la versione che preferisci
)

# Recupera redis++
FetchContent_Declare(
  redis-plus-plus
  GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
  GIT_TAG 1.3.7 # Sostituisci con la versione che preferisci
)

# Fetch jwt-cpp
# FetchContent_Declare(
#     jwt-cpp
#     GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp
#     GIT_TAG v0.7.1
# )

# Fetch libfort for ASCII data output in console
# FetchContent_Declare(
#     libfort
#     GIT_REPOSITORY https://github.com/seleznevae/libfort
#     GIT_TAG v0.4.2
# )

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog
    GIT_TAG v1.15.3
)

set(PQXX_BUILD_DOC OFF CACHE BOOL "Build documentation")
set(BUILD_TEST OFF CACHE BOOL "Build tests")
set(SKIP_BUILD_TEST ON CACHE BOOL "Skip build test")

FetchContent_MakeAvailable(libpqxx spdlog hiredis redis-plus-plus) #jwt-cpp libfort)

# Applica la patch automatica:
include("cmake/patch_redis_plus_plus_utils.cmake")


# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
# find_path(REDIS_PLUS_PLUS_HEADER sw)
# find_library(REDIS_PLUS_PLUS_LIB redis++)
# CPMAddPackage(
#   Crow
#   VERSION 1.2.1.2
#   GITHUB_REPOSITORY CrowCpp/Crow
#   OPTIONS
#     "CROW_BUILD_EXAMPLES Off"
#     "CROW_BUILD_TOOLS Off"
#     "CROW_BUILD_TESTS Off"
#     "CROW_BUILD_DOCS Off"
#     "CROW_ENABLE_COMPRESSION On"
#     "CROW_ENABLE_SSL Off"
# )

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/repository
    ${CMAKE_BINARY_DIR}  # Per config.hpp se necessario
    ${hiredis_SOURCE_DIR}
    ${redis-plus-plus_SOURCE_DIR}/src
)

target_link_libraries(
    ${PROJECT_NAME} PUBLIC
    spdlog::spdlog
    redis++::redis++
    hiredis
    # Crow::Crow
    pqxx
    # jwt-cpp
    PostgreSQL::PostgreSQL
    # libfort::fort
    # CURL::libcurl
    pthread
)

# target_link_libraries(
#   ${PROJECT_NAME} PRIVATE
#   Crow::Crow
#   jwt-cpp
#   pqxx
#   PostgreSQL::PostgreSQL
#   libfort::fort
#   CURL::libcurl
#   spdlog::spdlog
#   pthread
# )

# Installa la libreria (opzionale)
install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)
