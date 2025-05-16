cmake_minimum_required(VERSION 3.20)
project(waifu_gallery 
        VERSION 0.0.0
        LANGUAGES CXX)

# setup configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
find_package(xxHash CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)

# build
add_executable(waifu_gallery ${SRC_FILES})

target_link_libraries(waifu_gallery PRIVATE Qt6::Core Qt6::Widgets xxHash::xxhash)
