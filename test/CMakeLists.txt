add_executable(_test
      integration.cxx
      util.cxx
        )
find_package(modern_durak_game_option)
find_package(matchmaking_proxy)
find_package(Catch2)
target_link_libraries(_test
        myproject_options
        myproject_warnings
        Catch2::Catch2WithMain
        modern_durak_game_option::modern_durak_game_option
        matchmaking_proxy::matchmaking_proxy
        )
target_include_directories(_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})

add_test(NAME _test
        COMMAND _test -r junit)