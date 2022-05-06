cmake_minimum_required(VERSION 3.2)
project(lab17_ap CXX)

add_subdirectory("./dependencies/")
find_package(GLEW REQUIRED)
find_package(Freetype REQUIRED)

file(GLOB_RECURSE
    GL_FONT "./dependencies/glfont/src/"
)

add_executable(${PROJECT_NAME} Application.cpp ${GL_FONT})
target_include_directories(${PROJECT_NAME}
    PUBLIC "${GL_FONT}"
    PUBLIC "${GLEW_INCLUDE_DIRS}"
    PUBLIC "${FREETYPE_INCLUDE_DIRS}"
    .
)
target_link_libraries(${PROJECT_NAME}
    ${FREETYPE_LIBRARIES}
    glfw
    GLEW::GLEW
    glm::glm
    GL
)
target_precompile_headers(${PROJECT_NAME}
    PRIVATE PCH.h
)