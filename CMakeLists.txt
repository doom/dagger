cmake_minimum_required(VERSION 3.11)
project(dagger)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")

add_library(dagger INTERFACE)

target_sources(dagger INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/dagger.hpp)

target_include_directories(dagger INTERFACE include)

option(DAGGER_BUILD_TESTS "Build tests" OFF)

if (DAGGER_BUILD_TESTS)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()

    add_executable(dagger-tests
            tests/tests.cpp)

    target_link_libraries(dagger-tests dagger ${CONAN_LIBS})
endif ()
