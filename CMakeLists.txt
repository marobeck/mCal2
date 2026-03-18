cmake_minimum_required(VERSION 3.16)

project(mCal2 LANGUAGES CXX VERSION 0.2.0)

enable_testing()

add_subdirectory(client)
add_subdirectory(tests)