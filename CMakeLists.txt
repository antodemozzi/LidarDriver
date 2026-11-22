cmake_minimum_required(VERSION 3.16)
project(LidarDriver LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(LidarDriver
    src/main.cpp
    src/LidarDriver.cpp
    src/FileUtils.cpp
    src/DataSource.cpp
)

target_include_directories(LidarDriver PRIVATE include)

if(WIN32)
    target_link_libraries(LidarDriver PRIVATE ws2_32)
endif()

if(BUILD_TESTING)
    add_executable(LidarDriverTests
        tests/LidarDriverTests.cpp
        src/LidarDriver.cpp
        src/FileUtils.cpp
        src/DataSource.cpp
    )
    target_include_directories(LidarDriverTests PRIVATE include)
    if(WIN32)
        target_link_libraries(LidarDriverTests PRIVATE ws2_32)
    endif()
    add_test(NAME LidarDriverTests COMMAND LidarDriverTests)
endif()
