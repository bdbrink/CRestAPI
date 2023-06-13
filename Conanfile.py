from conans import ConanFile

class MyProjectConan(ConanFile):
    name = "myproject"
    version = "1.0.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake"
    requires = "boost/1.76.0"

    def configure(self):
        self.options["boost"].header_only = True

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        cmake = self._configure_cmake()
        cmake.install()

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake
