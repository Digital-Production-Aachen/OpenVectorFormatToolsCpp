set(EXAMPLE_NAME ex1_write)

add_executable(${EXAMPLE_NAME} main.cc)

target_include_directories(${EXAMPLE_NAME}
    PUBLIC
        ${PROJECT_SOURCE_DIR}/reader_writer/inc
)

target_link_libraries(${EXAMPLE_NAME}
    PRIVATE
        ${OVF_READER_WRITER_LIBRARY_STATIC}
)

# add defines for building static library
target_compile_definitions(${EXAMPLE_NAME}
    PRIVATE
        OVF_READER_WRITER_STATIC_DEFINE
)

# add defines for architecture
target_compile_definitions(${EXAMPLE_NAME}
    PRIVATE
        ${TARGET_ARCHITECTURE}
)