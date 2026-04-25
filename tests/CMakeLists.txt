cmake_minimum_required(VERSION 3.22)
project(qclocks_tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Catch2 downloaded as single-header (no install needed)
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

set(COMPONENT_ROOTS "${CMAKE_SOURCE_DIR}/../components")

# Shared include dirs (no ESP-IDF dependency)
set(COMMON_INCLUDES
    ${COMPONENT_ROOTS}/common
    ${COMPONENT_ROOTS}/word_layout
    ${COMPONENT_ROOTS}/clock_engine
)

# clock_engine test executable
add_executable(test_clock_engine
    clock_engine/test_clock_engine.cpp
    ${COMPONENT_ROOTS}/clock_engine/clock_engine.cpp
    ${COMPONENT_ROOTS}/clock_engine/clock_rules.cpp
    ${COMPONENT_ROOTS}/word_layout/word_layout.cpp
)
target_include_directories(test_clock_engine PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_clock_engine PRIVATE Catch2::Catch2WithMain)

# word_layout test executable
add_executable(test_word_layout
    word_layout/test_word_layout.cpp
    ${COMPONENT_ROOTS}/word_layout/word_layout.cpp
)
target_include_directories(test_word_layout PRIVATE ${COMMON_INCLUDES})
target_link_libraries(test_word_layout PRIVATE Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(test_clock_engine)
catch_discover_tests(test_word_layout)
