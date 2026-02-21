cmake_minimum_required(VERSION 3.14)
project(mCal2
        VERSION 0.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Collect all sources in src/
file(GLOB_RECURSE SOURCES src/*.cpp)

# Find libraries
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(SQLite3 REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCES})

# Link libaries
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets SQLite::SQLite3 uuid)

# Link headers
target_include_directories(${PROJECT_NAME} PRIVATE src src/data src/database src/GUI src/GUI/widgets src/database)

# --------------------------------------------------------------------------
# Copy icons into build assets directory so 'build' can be deleted and recreated
# --------------------------------------------------------------------------
set(ASSETS_SRC "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_DST "${CMAKE_BINARY_DIR}/assets")
if(EXISTS "${ASSETS_SRC}")
    # Ensure destination exists at configure time (so running cmake creates the folder)
    file(MAKE_DIRECTORY "${ASSETS_DST}")
    # Copy contents now at configure time so assets are available immediately after cmake
    file(COPY "${ASSETS_SRC}/" DESTINATION "${ASSETS_DST}")
else()
    message(WARNING "Asset source directory not found: ${ASSETS_SRC}. Skipping copy.")
endif()

# Also add a build-time target so assets are refreshed during 'cmake --build' or 'make'
add_custom_target(copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SRC}" "${ASSETS_DST}"
    COMMENT "Copying icons/ -> ${ASSETS_DST}"
)