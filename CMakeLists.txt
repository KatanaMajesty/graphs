cmake_minimum_required(VERSION 3.2)
project(lab17_ap CXX)

add_subdirectory("./dependencies/")
find_package(GLEW REQUIRED)

add_executable(${PROJECT_NAME} Application.cpp)
target_include_directories(${PROJECT_NAME}
    PUBLIC "${GLEW_INCLUDE_DIRS}"
    .
)
target_link_libraries(${PROJECT_NAME}
    IMGUI
    glfw
    GLEW::GLEW
    glm::glm
    GL
)
target_precompile_headers(${PROJECT_NAME}
    PRIVATE PCH.h
)