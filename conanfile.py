from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, CMakeDeps, cmake_layout


class OvfreaderwriterConan(ConanFile):
    name = "ovfreaderwriter"
    version = "0.0.1"
    requires = "protobuf/3.19.2"

    # Optional metadata
    license = "<Put the package license here>"
    author = "<Put your name here> <And your email here>"
    url = "<Package recipe repository url here, for issues about the package>"
    description = "<Description of Ovfreaderwriter here>"
    topics = ("<Put some tag here>", "<here>", "<and here>")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = ("CMakeLists.txt", "reader_writer/*", "test", "cmake/*", "OpenVectorFormat/*.proto")

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.settings.arch not in ("x86", "x86_64"):
            raise ConanInvalidConfiguration("Unsupported architecture")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.variables["ENABLE_TESTING"] = True
        tc.variables["ENABLE_EXAMPLES"] = False
        tc.variables["BUILD_STATIC_LIBS"] = not self.options.shared
        tc.variables["CMAKE_BUILD_TYPE"] = "Release" if self.settings.build_type == "Release" else "Debug"

        if self.settings.arch == "x86":
            tc.variables["TARGET_ARCHITECTURE"] = "_X86_"
        elif self.settings.arch == "x86_64":
            tc.variables["TARGET_ARCHITECTURE"] = "_AMD64_"

        tc.generate()

    def build(self):
        self.output.warn(self.deps_cpp_info["protobuf"].libdirs)

        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        # run ctest
        cmake.test()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["ovfreaderwriter"]
        
        # defaults to ["include"]
        #self.cpp_info.includedirs = ["include"]
        
        #if self.options.static:
        #    self.cpp_info.libs = ["protobuf?"]

