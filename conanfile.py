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
        self.options["boost"].header_only = True
        self.options["matchmaking_proxy"].with_log_co_spawn_print_exceptions = False
        self.options["matchmaking_proxy"].with_log_my_websocket = True
        self.options["matchmaking_proxy"].with_my_websocket_read_end = False
        self.options["matchmaking_proxy"].with_log_for_state_machine = True
        self.options["matchmaking_proxy"].with_log_object_to_string_with_object_name = True

    def requirements(self):
        self.requires("catch2/[<3]")
        self.requires("corrade/2020.06")
        self.requires("matchmaking_proxy/1.0.3")
        self.requires("modern_durak_game_option/latest")