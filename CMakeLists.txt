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

cmake_minimum_required(VERSION 3.16)

set(PROJECT "ReaderWriterCpp")

project("${PROJECT}_Project"
        VERSION 0.0.1
        LANGUAGES CXX)


set(OVF_READER_WRITER_LIBRARY_NAME OvfReaderWriter)
set(OVF_READER_WRITER_LIBRARY_STATIC  ${OVF_READER_WRITER_LIBRARY_NAME}_Static)
set(OVF_READER_WRITER_LIBRARY_DYNAMIC ${OVF_READER_WRITER_LIBRARY_NAME}_Dll)


# Global variables
set(CMAKE_CXX_STANDARD                  17)
set(CMAKE_CXX_STANDARD_REQUIRED         ON)
set(CMAKE_CXX_EXTENSIONS                OFF)

# Options
option(ENABLE_EXAMPLES    "Enables build of code examples."               ON)
option(ENABLE_TESTING     "Enable unit testing."                          ON)
option(BUILD_STATIC_LIBS  "Whether to build a static or dynamic library." ON)


# Cmake modules
include(GenerateExportHeader)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/)
include(config_safe_guards)

# Conan
include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake OPTIONAL
        RESULT_VARIABLE CONAN_FOUND)
if (CONAN_FOUND)
  conan_basic_setup()
else()
  message("Conan build information was not found. Libraries resolution will be against system include paths.")
endif()

# First-party library
add_subdirectory(reader_writer)

# Example applications
if (ENABLE_EXAMPLES)
  add_subdirectory(example)
endif()

# Unit test
if (ENABLE_TESTING)
  enable_testing()
  
  include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.0.0-preview3
  )

  FetchContent_MakeAvailable(Catch2)

  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

  include(CTest)
  include(Catch)

  add_subdirectory(test)
endif()

# Installation
#install(TARGETS ${OVF_READER_WRITER_LIBRARY_STATIC}
#        ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib/static
#        LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib/static
#        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/lib/static
#        PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/lib/static)

#install(TARGETS ${OVF_READER_WRITER_LIBRARY_DYNAMIC}
#        ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib/dll
#        LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib/dll
#        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/lib/dll
#        PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/lib/dll)