# xapp/test/CMakeLists.txt
message(STATUS "Configuring tests...")

# Opzione per abilitare il coverage
option(ENABLE_COVERAGE "Enable code coverage reporting" ON)

if(ENABLE_COVERAGE)
    message(STATUS "Code coverage enabled")
    add_compile_options(-fprofile-arcs -ftest-coverage)
    add_link_options(--coverage)
endif()

# Target eseguibile per i test
add_executable(${PROJECT_NAME}_tests
    test_main.cpp
    # Altri file di test
)

target_link_libraries(${PROJECT_NAME}_tests PRIVATE
    libxapp
    Catch2::Catch2WithMain
)

target_compile_definitions(xapp_tests PRIVATE $<TARGET_PROPERTY:xapp,COMPILE_DEFINITIONS>)

target_include_directories(${PROJECT_NAME}_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
)

add_test(
    NAME ${PROJECT_NAME}_tests
    COMMAND ${PROJECT_NAME}_tests
)

# Configurazione coverage
if(ENABLE_COVERAGE)
    find_program(GCOVR_EXECUTABLE gcovr)
    find_program(LCOV_EXECUTABLE lcov)
    find_program(GENHTML_EXECUTABLE genhtml)

    if(GCOVR_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${GCOVR_EXECUTABLE}
                --gcov-executable "llvm-cov-18 gcov"
                --root ${CMAKE_SOURCE_DIR}
                --exclude ".*/tests/.*"
                --exclude ".*/_deps/.*"
                --html --html-details -o ${CMAKE_BINARY_DIR}/coverage.html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${PROJECT_NAME}_tests  # <-- Aggiungi questa linea
            COMMENT "Generating coverage report with gcovr..."
        )
    elseif(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE)
        add_custom_target(coverage
            COMMAND ${LCOV_EXECUTABLE} --directory . --capture --output-file coverage.info
            COMMAND ${LCOV_EXECUTABLE} --remove coverage.info '/usr/*' '*/tests/*' --output-file coverage.info
            COMMAND ${GENHTML_EXECUTABLE} coverage.info --output-directory ${CMAKE_BINARY_DIR}/coverage
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS ${PROJECT_NAME}_tests  # <-- Aggiungi questa linea
            COMMENT "Generating coverage report with lcov..."
        )
    else()
        message(WARNING "Code coverage tools not found. Install gcovr with: pip install gcovr")
    endif()
endif()



# find_program(GCOVR_EXECUTABLE gcovr)
# if(GCOVR_EXECUTABLE)
#     add_custom_target(coverage
#         COMMAND ${GCOVR_EXECUTABLE}
#             --root ${CMAKE_SOURCE_DIR}
#             --exclude ".*/tests/.*"
#             --html-details --output coverage.html
#         WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#     )
# endif()




# # xapp/test/CMakeLists.txt
# message(STATUS "Configuring tests...")

# # Crea un target eseguibile per i test
# add_executable(${PROJECT_NAME}_tests
#     test_main.cpp
#     # Aggiungi altri file di test qui
# )

# # Collega alla libreria principale e a Catch2
# target_link_libraries(${PROJECT_NAME}_tests PRIVATE
#     libxapp                  # La tua libreria principale
#     Catch2::Catch2WithMain   # Framework di test
# )

# # Assicurati che erediti le definizioni
# target_compile_definitions(xapp_tests PRIVATE $<TARGET_PROPERTY:xapp,COMPILE_DEFINITIONS>)

# # Aggiungi include directory se necessario
# target_include_directories(${PROJECT_NAME}_tests PRIVATE
#     ${CMAKE_SOURCE_DIR}/src/include
# )

# # Aggiungi i test a CTest
# add_test(
#     NAME ${PROJECT_NAME}_tests
#     COMMAND ${PROJECT_NAME}_tests
# )
