project("depths_of_hoenn")


set(SOURCES "main.cpp" "../core/camera.h" "../core/shader.h")

add_compile_definitions(PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/../core/objects")
add_compile_definitions(PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/../core/textures")

add_executable(${PROJECT_NAME} ${SOURCES})
find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL glfw glad assimp)