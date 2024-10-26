from conan import ConanFile
from conan.tools.cmake import cmake_layout

class Pkg(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain"

    def requirements(self):
        self.requires("fmt/10.2.1")
        self.requires("spdlog/1.14.1")
        self.requires("cpptrace/0.7.0", force=True)
        self.requires("libassert/2.1.0")
        self.requires("mongo-cxx-driver/3.8.1")
        self.requires("unordered_dense/4.4.0")
        self.requires("duckdb/1.1.2")
        self.requires("spscqueue/1.1")
        self.requires("lyra/1.6.1")

        # conflicts
        self.requires("zstd/1.5.6", override=True)

    def build_requirements(self):
        self.test_requires("gtest/1.14.0")
        self.test_requires("benchmark/1.9.0")
        self.test_requires("xoshiro-cpp/1.1")

    def configure(self):
        self.options["mongo-cxx-driver"].polyfill = "std"

    def layout(self):
        cmake_layout(self)
