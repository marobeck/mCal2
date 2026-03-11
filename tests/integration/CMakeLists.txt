find_package(Python3 REQUIRED)

add_test(
    NAME sync_test
    COMMAND Python3::Interpreter
        ${CMAKE_CURRENT_SOURCE_DIR}/run_sync_test.py
)