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
set(CMAKE_AUTOUIC_SEARCH_PATHS ${PROJECT_SOURCE_DIR}/src/ui)

set(CMAKE_BUILD_TYPE Debug)

set(VCPKG_APPLOCAL_DEPS OFF)

# libs
find_package(Qt6 COMPONENTS Core Widgets Sql REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Stb REQUIRED)
set(PROJECT_LIBS Qt6::Core Qt6::Widgets Qt6::Sql xxHash::xxhash WebP::webp WebP::webpdecoder WebP::webpdemux nlohmann_json::nlohmann_json unofficial::sqlite3::sqlite3)

# src files
set(PROJECT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp) # remember configure CMake before build
file(GLOB UI_FILES ${PROJECT_SOURCE_DIR}/src/ui/*.ui)
file(GLOB RES_FILES ${PROJECT_SOURCE_DIR}/res/*.qrc)
file(GLOB RC_FILES ${PROJECT_SOURCE_DIR}/res/*.rc)

# target
add_executable(waifu_gallery WIN32 ${SRC_FILES} ${UI_FILES} ${RES_FILES} ${RC_FILES})
target_include_directories(waifu_gallery PRIVATE ${PROJECT_INCLUDE_DIRS})
target_link_libraries(waifu_gallery PRIVATE ${PROJECT_LIBS})

# test
if(BUILD_TEST_TARGET STREQUAL "ON")
    file(GLOB_RECURSE TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/*.cpp)
    list(REMOVE_ITEM TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/src/main.cpp) # exclude main.cpp
    file(GLOB TEST_FILES ${PROJECT_SOURCE_DIR}/tests/*.cpp) # include test files
    add_executable(TEST_waifu_gallery
        ${TEST_SRC_FILES}
        ${TEST_FILES}
    )
    target_include_directories(TEST_waifu_gallery PRIVATE ${PROJECT_INCLUDE_DIRS})
    target_link_libraries(TEST_waifu_gallery PRIVATE ${PROJECT_LIBS})
endif()