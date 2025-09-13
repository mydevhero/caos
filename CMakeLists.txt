cmake_minimum_required(VERSION 3.5)

cmake_policy(SET CMP0066 NEW)
cmake_policy(SET CMP0102 NEW)
cmake_policy(SET CMP0126 NEW)
cmake_policy(SET CMP0128 NEW)

# set(CMAKE_C_COMPILER "clang")
# set(CMAKE_CXX_COMPILER "clang++")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project Name
project(caos VERSION 0.0.1 LANGUAGES CXX)

# Aggiungi la sottodirectory della libreria
add_subdirectory(libcaos)

add_library(repoexception STATIC
    libcaos/Middleware/Repository/Exception.hpp
)

target_include_directories(repoexception PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/libcaos/Middleware/Repository
)
target_link_libraries(repoexception PUBLIC libcaos)

add_executable(${PROJECT_NAME} src/${PROJECT_NAME}.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE libcaos repoexception)

add_dependencies(${PROJECT_NAME} increment_build)

CPMAddPackage(
  Crow
  VERSION 1.2.1.2
  GITHUB_REPOSITORY CrowCpp/Crow
  OPTIONS
    "CROW_BUILD_EXAMPLES Off"
    "CROW_BUILD_TOOLS Off"
    "CROW_BUILD_TESTS Off"
    "CROW_BUILD_DOCS Off"
    "CROW_ENABLE_COMPRESSION On"
    "CROW_ENABLE_SSL Off"
)

# configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.hpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR})

target_link_libraries(
    ${PROJECT_NAME} PUBLIC
    Crow::Crow
)


if(ENABLE_COVERAGE)
    # target_compile_options(libcaos PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    # target_link_options(libcaos PRIVATE -fprofile-instr-generate -fcoverage-mapping)

    target_compile_options(caos PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(caos PRIVATE -fprofile-instr-generate -fcoverage-mapping)

    # target_compile_options(${PROJECT_NAME}_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
    # target_link_options(${PROJECT_NAME}_tests PRIVATE -fprofile-instr-generate -fcoverage-mapping)
endif()


#option(${PROJECT_NAME}_ENABLE_TESTS "Enable testing" OFF)
if(${PROJECT_NAME}_ENABLE_TESTS)

  target_compile_definitions(caos PRIVATE caos_ENABLE_TESTS)
  message(STATUS "Testing features enabled - caos_ENABLE_TESTS macro defined")

  include(CTest)
  enable_testing()

  CPMAddPackage(
    Catch2
    VERSION 3.9.0
    GITHUB_REPOSITORY catchorg/Catch2
    OPTIONS
    "CATCH_INSTALL_DOCS Off"
    "CATCH_INSTALL_EXTRAS Off"
    "CATCH_ENABLE_REPRODUCIBLE_BUILD Off"
  )

  add_subdirectory(test)
endif()
