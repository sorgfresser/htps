cmake_minimum_required(VERSION 3.25)
project(htps)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


find_package(Python COMPONENTS Interpreter Compiler Development)


add_executable(htps src/main.cpp src/graph/base.cpp src/graph/graph.cpp src/graph/htps.cpp src/model/policy.cpp)
target_include_directories(htps PRIVATE external/glob/single_include)
add_executable(pythonhtps python/htps.cpp src/graph/lean.cpp src/graph/htps.cpp src/model/policy.cpp src/graph/base.cpp src/graph/graph.cpp)
target_include_directories(pythonhtps PRIVATE external/glob/single_include)

target_compile_definitions(pythonhtps PRIVATE PYTHON_BINDINGS)
target_link_libraries(pythonhtps PRIVATE Python::Python)

add_executable(test tests/htps_tests.cpp src/graph/base.cpp src/graph/graph.cpp src/graph/htps.cpp src/model/policy.cpp)
target_include_directories(test PRIVATE external/glob/single_include)

target_link_libraries(test gtest_main)
include(GoogleTest)
gtest_discover_tests(test)
