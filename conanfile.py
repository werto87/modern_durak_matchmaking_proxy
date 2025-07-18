from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class Project(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators =  "CMakeDeps"


    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False #workaround because this leads to useless options in cmake-tools configure
        tc.generate()

    def configure(self):
        # We can control the options of our dependencies based on current options
        self.options["catch2"].with_main = True
        self.options["catch2"].with_benchmark = True
        self.options["matchmaking_proxy"].with_log_for_state_machine = False
        self.options["matchmaking_proxy"].with_log_object_to_string_with_object_name = False
        self.options["my_web_socket"].log_co_spawn_print_exception = True
        self.options["my_web_socket"].log_write = True
        self.options["my_web_socket"].log_read = True





    def requirements(self):
        self.requires("boost/1.86.0",force=True)
        self.requires("catch2/[<3]")
        self.requires("corrade/2025.06")
        self.requires("matchmaking_proxy/1.2.5")
        self.requires("modern_durak_game_option/latest")
        self.requires("confu_algorithm/1.2.1")
        self.requires("magic_enum/0.9.6",force=True)
         