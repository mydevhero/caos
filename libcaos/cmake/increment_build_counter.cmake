# Percorsi
set(BUILD_COUNTER_FILE "${CMAKE_BINARY_DIR}/build_counter.txt")
set(VERSION_HEADER_IN   "${CMAKE_CURRENT_LIST_DIR}/version.h.in")
set(VERSION_HEADER_OUT  "${CMAKE_BINARY_DIR}/version.hpp")

# Timestamp
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")

# Build counter
if(EXISTS "${BUILD_COUNTER_FILE}")
    file(READ "${BUILD_COUNTER_FILE}" BUILD_VERSION)
    string(STRIP "${BUILD_VERSION}" BUILD_VERSION)
    math(EXPR BUILD_VERSION "${BUILD_VERSION} + 1")
else()
    set(BUILD_VERSION 1)
endif()

file(WRITE "${BUILD_COUNTER_FILE}" "${BUILD_VERSION}")

# Sostituisci e scrivi version.hpp
configure_file(
  "${VERSION_HEADER_IN}"
  "${VERSION_HEADER_OUT}"
  @ONLY
)
