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
    requires = [
        "coco-ledstrip/main",
        "coco-noise/main",
        "coco-monodisplay/pow10",
        "coco-flash/pow10",
        "coco-storage/pow10"
    ]


    # check if we are cross compiling
    def cross(self):
        if hasattr(self, "settings_build"):
            return self.settings.os != self.settings_build.os
        return False

    def build_requirements(self):
        self.tool_requires("coco-toolchain/pow10", options={"platform": self.options.platform})
        #self.test_requires("coco-devboards/pow10", options={"platform": self.options.platform})

    def configure(self):
        # pass platform option to dependencies
        self.options["coco/*"].platform = self.options.platform
        self.options["coco-loop/*"].platform = self.options.platform
        self.options["coco-buffer/*"].platform = self.options.platform
        self.options["coco-ledstrip/*"].platform = self.options.platform
        self.options["coco-noise/*"].platform = self.options.platform
        self.options["coco-monodisplay/*"].platform = self.options.platform
        self.options["coco-flash/*"].platform = self.options.platform
        self.options["coco-storage/*"].platform = self.options.platform

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
