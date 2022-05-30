# OpenVectorFormatToolsCpp

This repository mainly contains a c++ file reader/writer implementation for the [open vector format](https://github.com/Digital-Production-Aachen/OpenVectorFormat).


## Usage

The general usage is very much similar to [the c# implementation found here](https://github.com/Digital-Production-Aachen/OpenVectorFormatTools/tree/main/ReaderWriter#how-to-use).

Basic examples can be found [in the `examples` directory](/example). Reference documentation is done as docstrings in the header files (and should show up in most IDE integrations). See [`ovf_file_reader.h`](/reader_writer/inc/ovf_file_reader.h) and [`ovf_file_writer.h`](/reader_writer/inc/ovf_file_writer.h) for details.


## Requirements

This project uses cmake 3.16 or higher as its build system.

With OVF implemented in protobuf, this project is dependant on protobuf as well. By default, the dependency is added with conan, but other ways of providing the library are supported. If no conan build information is found, cmake will only issue a message. As long as protobuf can be found from a `find_package(protobuf)` call, the build should work.


## Build instructions

Note that during the first call to `cmake`, the build system generation might emit a few errors regarding `open_vector_format.pb.h` - these error messages can be ignored. The file in question will be generated in the build step.

Debug build:
```sh
mkdir build && cd build

# if dependencies are to be provided via conan
conan install .. -s build_type=Debug

cmake .. -DCMAKE_BUILD_TYPE=Debug

cmake --build .
```

Release build:
```sh
mkdir build && cd build

# if dependencies are to be provided via conan
conan install .. -s build_type=Release

cmake .. -DCMAKE_BUILD_TYPE=Release

cmake --build .
```



## Attribution

Uses following open-source contents:
- [Parts of Cpp-Project-Template by Jan Schaffranek (MIT)](https://github.com/franneck94/Cpp-Project-Template/)
- [LTO cmake module by lectem (MIT)](https://github.com/Lectem/cpp-boilerplate/)
