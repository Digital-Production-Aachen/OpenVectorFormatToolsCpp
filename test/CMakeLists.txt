set(TEST_NAME tests)

add_executable(${TEST_NAME}
    test_reader.cc
    test_writer.cc
)

target_include_directories(${TEST_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/reader_writer/inc
)

target_link_libraries(${TEST_NAME}
    PRIVATE
        ${OVF_READER_WRITER_LIBRARY_STATIC}
        Catch2::Catch2WithMain
)

# add defines for building static library
target_compile_definitions(${TEST_NAME}
    PRIVATE
        OVF_READER_WRITER_STATIC_DEFINE
)

# add defines for architecture
target_compile_definitions(${TEST_NAME}
    PRIVATE
        ${TARGET_ARCHITECTURE}
)

catch_discover_tests(${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources
)