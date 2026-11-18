cmake_minimum_required(VERSION 3.16)

project(TheWishGoneWrong LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(TheWishGoneWrong
    src/main.cpp
    src/glad.c
)

target_include_directories(TheWishGoneWrong PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(TheWishGoneWrong PRIVATE
    OpenGL::GL
    glfw
)
