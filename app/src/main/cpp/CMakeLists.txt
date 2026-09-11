cmake_minimum_required(VERSION 3.15)
project("souliano_native")

# Enable FetchContent for retrieving Oboe
include(FetchContent)

FetchContent_Declare(
  oboe
  GIT_REPOSITORY https://github.com/google/oboe.git
  GIT_TAG refs/tags/v1.7.3
)

FetchContent_MakeAvailable(oboe)

add_library(souliano_native SHARED
    native-lib.cpp
    AudioEngine.cpp
    WAVWriter.cpp
)

find_package(oboe CONFIG REQUIRED)

target_include_directories(souliano_native PRIVATE ${oboe_SOURCE_DIR}/include)

target_link_libraries(souliano_native PRIVATE oboe::oboe)

# Use c++17
set_target_properties(souliano_native PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
