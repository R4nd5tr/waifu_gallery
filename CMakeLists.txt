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

find_package(Qt6 COMPONENTS Core Widgets Sql REQUIRED)
find_package(xxHash CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp) # remember configure CMake before build
set(TEST_SRC_FILES ${SRC_FILES})
list(REMOVE_ITEM SRC_FILES ${PROJECT_SOURCE_DIR}/src/test.cpp) # remove test entrance
list(REMOVE_ITEM TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp) # remove main entrance

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(PROJECT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_LIBS Qt6::Core Qt6::Widgets Qt6::Sql xxHash::xxhash)

set(VCPKG_APPLOCAL_DEPS OFF)

# targets
# add_executable(waifu_gallery ${SRC_FILES})
# target_include_directories(waifu_gallery PRIVATE ${PROJECT_INCLUDE_DIRS})
# target_link_libraries(waifu_gallery PRIVATE ${PROJECT_LIBS})

add_executable(test ${TEST_SRC_FILES})
target_include_directories(test PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(test PRIVATE ${PROJECT_LIBS})