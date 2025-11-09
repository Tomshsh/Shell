cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

# Set C++ standard globally (or use target_compile_features below)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Create libraries
add_library(command src/Command.cpp)
target_include_directories(command PUBLIC include)
target_compile_features(command PUBLIC cxx_std_23)

add_library(parser src/Parser.cpp)
target_include_directories(parser PUBLIC include)
target_compile_features(parser PUBLIC cxx_std_23)

target_link_libraries(parser PUBLIC command)  # ← parser depends on command

# Create executable (list files explicitly, excluding Command.cpp and Parser.cpp)
add_executable(shell 
    src/main.cpp
    # Add other source files here explicitly
)

target_link_libraries(shell PRIVATE readline command parser)
target_compile_features(shell PRIVATE cxx_std_23)