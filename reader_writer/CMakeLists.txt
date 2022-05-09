set(PROTO_BASE_PATH ${PROJECT_SOURCE_DIR}/OpenVectorFormat)

find_package(Protobuf REQUIRED)
set(Protobuf_IMPORT_DIRS ${PROTO_BASE_PATH})
protobuf_generate_cpp(
    PROTO_SRCS
    PROTO_HDRS
    EXPORT_MACRO OVF_READER_WRITER_EXPORT
    ${PROTO_BASE_PATH}/open_vector_format.proto
    ${PROTO_BASE_PATH}/ovf_lut.proto
)

if (MSVC)
    set(FORCE_INCLUDE_FLAG "/FI")
else()
    set(FORCE_INCLUDE_FLAG "-include")
endif()

set(EXPORT_HEADER_BASE_NAME ovf_reader_writer)

set(PUBLIC_HEADER_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/ovf_file_reader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/ovf_file_writer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/memory_mapping_win32.h
    ${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_HEADER_BASE_NAME}_export.h
    "${PROTO_HDRS}"
)

### STATIC LIBRARY ###
if (1)

    add_library(${OVF_READER_WRITER_LIBRARY_STATIC} STATIC)
    generate_export_header(${OVF_READER_WRITER_LIBRARY_STATIC}
        BASE_NAME ${EXPORT_HEADER_BASE_NAME}
    )

    # add defines for building static library
    target_compile_definitions(${OVF_READER_WRITER_LIBRARY_STATIC}
        PRIVATE
            OVF_READER_WRITER_STATIC_DEFINE
    )

    # add defines for architecture
    target_compile_definitions(${OVF_READER_WRITER_LIBRARY_STATIC}
        PRIVATE
            ${TARGET_ARCHITECTURE}
    )

    # force include dllspec export defines so they work in generated files
    target_compile_options(${OVF_READER_WRITER_LIBRARY_STATIC}
        PRIVATE
            ${FORCE_INCLUDE_FLAG}${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_HEADER_BASE_NAME}_export.h
    )

    target_sources(${OVF_READER_WRITER_LIBRARY_STATIC}
        PRIVATE
            src/ovf_file_reader.cc
            src/ovf_file_writer.cc
            src/util.cc
            ${PROTO_SRCS}
        PUBLIC
            ${PROTO_HDRS}
    )

    target_include_directories(${OVF_READER_WRITER_LIBRARY_STATIC}
        PUBLIC
            inc
            ${CMAKE_CURRENT_BINARY_DIR}
        PRIVATE
            ${Protobuf_INCLUDE_DIRS}
            ${CMAKE_INCLUDE_PATH}
    )

    set_target_properties(${OVF_READER_WRITER_LIBRARY_STATIC}
        PROPERTIES
            PUBLIC_HEADER "${PUBLIC_HEADER_LIST}"
    )

    target_link_libraries(${OVF_READER_WRITER_LIBRARY_STATIC}
        PRIVATE
            ${Protobuf_LIBRARIES}
    )
endif()


### SHARED ###
if (0)

    add_library(${OVF_READER_WRITER_LIBRARY_DYNAMIC} SHARED)
    generate_export_header(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        BASE_NAME ovf_reader_writer
    )

    # set default dll symbol export visibility 
    set_target_properties(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        PROPERTIES
            C_VISIBILITY_PRESET         hidden
            CXX_VISIBILITY_PRESET       hidden
            VISIBILITY_INLINES_HIDDEN   1
    )

    # force include dllspec export defines so they work in generated files
    target_compile_options(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        PRIVATE
            ${FORCE_INCLUDE_FLAG}${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_HEADER_BASE_NAME}_export.h
    )

    # add defines for architecture
    target_compile_definitions(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        PRIVATE
            ${TARGET_ARCHITECTURE}
    )

    target_sources(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        PRIVATE
            src/ovf_file_reader.cc
            src/ovf_file_writer.cc
            src/util.cc
            ${PROTO_SRCS}
    )

    target_include_directories(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        PUBLIC
            inc
            ${CMAKE_CURRENT_BINARY_DIR}
        PRIVATE
            ${Protobuf_INCLUDE_DIRS}
            ${CMAKE_INCLUDE_PATH}
    )

    set_target_properties(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        PROPERTIES
            PUBLIC_HEADER "${PUBLIC_HEADER_LIST}"
    )

    target_link_libraries(${OVF_READER_WRITER_LIBRARY_DYNAMIC}
        PRIVATE
            ${Protobuf_LIBRARIES}
    )
endif()