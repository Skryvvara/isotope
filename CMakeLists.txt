cmake_minimum_required(VERSION 3.5)
project(isotope)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set the version variable
set(VERSION "v0.0.1-dev")

# Add compile options
add_compile_options(-Wall -std=c++11 -DVERSION="${VERSION}")

# Add your source files
set(SOURCES
    src/main.cpp
)

# Create executable
add_executable(isotope ${SOURCES})
