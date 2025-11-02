cmake_minimum_required(VERSION 3.20)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_LIST_DIR}/../../hello_lib)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app)

target_sources(app PRIVATE
src/test_ckks_params.c
src/test_ntt.c
)
