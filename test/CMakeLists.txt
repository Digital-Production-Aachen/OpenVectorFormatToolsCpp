#[[
---- Copyright Start ----

MIT License

Copyright (c) 2022 Digital-Production-Aachen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

---- Copyright End ----
]]

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