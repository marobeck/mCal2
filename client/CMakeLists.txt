file(GLOB_RECURSE CLIENT_SRC CONFIGURE_DEPENDS src/*.cpp)

# Remove main.cpp from the library sources
list(REMOVE_ITEM CLIENT_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
message(STATUS "CLIENT SRC: ${CLIENT_SRC}")

add_library(mcal_client ${CLIENT_SRC})

target_compile_features(mcal_client PUBLIC cxx_std_17)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(SQLite3 REQUIRED)

# Set properties for Qt's MOC, UIC, and RCC
set_target_properties(mcal_client PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

target_link_libraries(mcal_client
    PUBLIC
    Qt5::Widgets
    Qt5::Network
    SQLite::SQLite3
    uuid
)

target_include_directories(mcal_client
    PUBLIC
    src
    src/data
    src/database
    src/GUI
    src/GUI/widgets
)

# Executable
add_executable(mcal2 src/main.cpp)

target_link_libraries(mcal2 PRIVATE mcal_client)

# Copy assets
add_custom_command(TARGET mcal2 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:mcal2>/assets
)