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

add_executable(htps src/main.cpp src/graph/base.cpp src/graph/graph.cpp src/tokenization/tokenizer.cpp src/tokenization/tokenization_utils.cpp src/graph/htps.cpp src/model/policy.cpp)

add_executable(tokenizer src/tokenization/create_tokenizer.cpp src/tokenization/tokenization_utils.cpp)

add_executable(test tests/htps_tests.cpp src/graph/base.cpp src/graph/graph.cpp src/tokenization/tokenizer.cpp src/tokenization/tokenization_utils.cpp src/graph/htps.cpp src/model/policy.cpp)

add_subdirectory(Tokenizer)
target_link_libraries(htps OpenNMTTokenizer)
target_link_libraries(tokenizer OpenNMTTokenizer)
target_link_libraries(test OpenNMTTokenizer)

add_subdirectory(glob)
target_link_libraries(htps Glob)
target_link_libraries(tokenizer Glob)
target_link_libraries(test Glob)

target_link_libraries(test gtest_main)
include(GoogleTest)
gtest_discover_tests(test)
