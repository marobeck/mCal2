cmake_minimum_required(VERSION 3.14)
project(mCal2
        VERSION 0.0
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Collect all sources in src/
file(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

# Find libraries
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(SQLite3 REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

# Link libaries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets SQLite::SQLite3 uuid)

# Link headers
target_include_directories(${PROJECT_NAME} PRIVATE src)