from conan import ConanFile
from conan.tools.files import copy
from conan.tools.cmake import CMake


class Project(ConanFile):
    name = "LedControl"
    description = "Control unit for a color LED strip"
    url = "https://github.com/Jochen0x90h/LedControl"
    license = "MIT"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "platform": [None, "ANY"]}
    default_options = {
        "platform": None}
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "conanfile.py", "CMakeLists.txt", "coco/*", "test/*"


    # check if we are cross compiling
    def cross(self):
        if hasattr(self, "settings_build"):
            return self.settings.os != self.settings_build.os
        return False

    def requirements(self):
        self.requires("coco-led-strip/towards1", options={"platform": self.options.platform})
        self.requires("coco-spi/towards1", options={"platform": self.options.platform})
        self.requires("coco-mono-display/towards1", options={"platform": self.options.platform})
        self.requires("coco-flash/towards1", options={"platform": self.options.platform})
        self.requires("coco-storage/towards1", options={"platform": self.options.platform})
        self.requires("coco-math/towards1", options={"platform": self.options.platform})
        self.requires("coco-ir/main", options={"platform": self.options.platform})

    def build_requirements(self):
        self.tool_requires("coco-toolchain/towards1", options={"platform": self.options.platform})
        self.test_requires("coco-devboards/towards1", options={"platform": self.options.platform})
        if not self.cross():
            self.test_requires("gtest/1.15.0")

    keep_imports = True
    def imports(self):
        # copy dependent libraries into the build folder
        copy(self, "*", src="@bindirs", dst="bin")
        copy(self, "*", src="@libdirs", dst="lib")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

        # run unit tests if CONAN_RUN_TESTS environment variable is set to 1
        #if os.getenv("CONAN_RUN_TESTS") == "1" and not self.cross():
        #    cmake.test()

    def package(self):
        # install from build directory into package directory
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        pass
