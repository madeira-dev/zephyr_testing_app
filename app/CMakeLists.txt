cmake_minimum_required(VERSION 3.20)

# 1. Load your embedded OpenFHE module
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../hello_lib)

# 2. Initialize Zephyr
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# 3. Define Project
project(app LANGUAGES C CXX)

# 4. Add Sources
target_sources(app PRIVATE src/initial_test.cpp)
