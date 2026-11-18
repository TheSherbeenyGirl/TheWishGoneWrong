cmake_minimum_required(VERSION 3.16)

project(TheWishGoneWrong LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(OpenCV REQUIRED)

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
    glm::glm
    assimp::assimp
)
add_executable(assimp_test src/assimp_test.cpp)

target_link_libraries(assimp_test PRIVATE
    assimp::assimp
)
add_executable(model_viewer
    src/model_viewer.cpp
    src/glad.c
)

target_include_directories(model_viewer PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(model_viewer PRIVATE
    OpenGL::GL
    glfw
    glm::glm
    assimp::assimp
    ${OpenCV_LIBS}
)
