cmake_minimum_required(VERSION 3.25)
project(htps)

set(CMAKE_CXX_STANDARD 23)

add_executable(htps src/main.cpp src/graph/base.cpp src/graph/graph.cpp src/tokenization/tokenizer.cpp src/tokenization/tokenization_utils.cpp src/graph/mcts.cpp src/model/policy.cpp)

add_executable(tokenizer src/tokenization/create_tokenizer.cpp src/tokenization/tokenization_utils.cpp)

add_subdirectory(Tokenizer)
target_link_libraries(htps OpenNMTTokenizer)
target_link_libraries(tokenizer OpenNMTTokenizer)

add_subdirectory(glob)
target_link_libraries(htps Glob)
target_link_libraries(tokenizer Glob)
