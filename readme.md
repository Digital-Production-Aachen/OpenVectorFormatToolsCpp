

Debug build:
```sh
mkdir build && cd build
conan install .. -s build_type=Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

Release build:
```sh
mkdir build && cd build
conan install .. -s build_type=Debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

Uses following open-source contents:
- [Parts of Cpp-Project-Template by Jan Schaffranek (MIT)](https://github.com/franneck94/Cpp-Project-Template/)
- [LTO cmake module by lectem (MIT)](https://github.com/Lectem/cpp-boilerplate/)
