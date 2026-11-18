cmake_minimum_required(VERSION 3.16)

project(TheWishGoneWrong)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(TheWishGoneWrong
    src/main.cpp
)

target_link_libraries(TheWishGoneWrong
    PRIVATE
    OpenGL::GL
    glfw
)
