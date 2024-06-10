##
## Copyright (c) 2024 Waldemar Schneider (w-schneider1987 at web dot de)
##
## Distributed under the Boost Software License, Version 1.0.
## (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)


add_library(cxx_template hello.cxx)


add_library(myproject::cxx_template ALIAS cxx_template)


find_package(Boost)
target_link_libraries(cxx_template PUBLIC
        boost::boost
)

target_include_directories(cxx_template INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>)

target_compile_features(cxx_template PUBLIC cxx_std_20)

set_target_properties(
        cxx_template
        PROPERTIES VERSION ${PROJECT_VERSION}
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES)

install(FILES
        hello.hxx
        DESTINATION include/cxx_template
        )
install(TARGETS cxx_template DESTINATION lib)
