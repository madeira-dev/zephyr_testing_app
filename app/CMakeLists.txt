cmake_minimum_required(VERSION 3.20)

# Convert relative path to absolute path
get_filename_component(HELLO_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../hello_lib" ABSOLUTE)

# This MUST come BEFORE find_package(Zephyr)
set(ZEPHYR_EXTRA_MODULES ${HELLO_LIB_PATH})

# This command finds and initializes Zephyr and all its modules
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)

# Add sources to the 'app' target that Zephyr created
target_sources(app PRIVATE src/main.c)

# Link against your library
target_link_libraries(app PRIVATE hello_lib)