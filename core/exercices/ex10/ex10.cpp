#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <map>

#include "../../camera.h"
#include "../../shader.h"
#include "../object.h"

const int width = 500;
const int height = 500;

GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);

void loadCubemapFace(const char* file, const GLenum& targetCube);

#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    std::cout << "Debug message (" << id << "): " << message << std::endl;
}
#endif

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

int main(int argc, char* argv[])
{
    std::cout << "Welcome to exercice 10: Implement refraction on an object" << std::endl;

    if (!glfwInit()) throw std::runtime_error("Failed to initialise GLFW \n");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "Exercise 10", nullptr, nullptr);
    if (window == NULL) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window\n");
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("Failed to initialize GLAD");

    glEnable(GL_DEPTH_TEST);

    // ---------------------------------------------------------
    // SHADER 1: THE REFRACTIVE SPHERE
    // ---------------------------------------------------------
    const std::string sourceV = "#version 330 core\n"
        "in vec3 position; \n"
        "in vec2 tex_coords; \n"
        "in vec3 normal; \n"
        "out vec3 v_frag_coord; \n"
        "out vec3 v_normal; \n"
        "uniform mat4 M; \n"
        "uniform mat4 itM; \n"
        "uniform mat4 V; \n"
        "uniform mat4 P; \n"
        "void main(){ \n"
        "   vec4 frag_coord = M * vec4(position, 1.0); \n"
        "   gl_Position = P * V * frag_coord; \n"
        "   v_normal = vec3(itM * vec4(normal, 1.0)); \n"
        "   v_frag_coord = frag_coord.xyz; \n"
        "}\n";

    // Refraction is coded in the fragment shader
    const std::string sourceF = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "precision mediump float; \n"
        "in vec3 v_frag_coord; \n"
        "in vec3 v_normal; \n"
        "uniform vec3 u_view_pos; \n"
        
        // Uniforms for texture and material density
        "uniform samplerCube cubemapSampler; \n"
        "uniform float refractionRatio; \n"

        "void main() { \n"
        // Calculate view direction and normalize normal
        "   vec3 I = normalize(v_frag_coord - u_view_pos); \n"
        "   vec3 N = normalize(v_normal); \n"
        
        // Calculate the refraction vector R using GLSL refract method
        "   vec3 R = refract(I, N, refractionRatio); \n"
        
        // Color the fragment using the cubemap texture
        "   FragColor = vec4(texture(cubemapSampler, R).rgb, 1.0); \n"
        "} \n";

    Shader shader(sourceV, sourceF);

    // ---------------------------------------------------------
    // SHADER 2: THE SKYBOX CUBEMAP
    // ---------------------------------------------------------
    const std::string sourceVCubeMap = "#version 330 core\n"
        "in vec3 position; \n"
        "in vec2 tex_coords; \n"
        "in vec3 normal; \n"
        "uniform mat4 V; \n"
        "uniform mat4 P; \n"
        "out vec3 texCoord_v; \n"
        "void main(){ \n"
        "   texCoord_v = position;\n"
        "   mat4 V_no_rot = mat4(mat3(V));\n"
        "   vec4 pos = P * V_no_rot * vec4(position, 1.0); \n"
        "   gl_Position = pos.xyww;\n"
        "}\n";

    const std::string sourceFCubeMap =
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "precision mediump float; \n"
        "uniform samplerCube cubemapSampler; \n"
        "in vec3 texCoord_v; \n"
        "void main() { \n"
        "   FragColor = texture(cubemapSampler, texCoord_v); \n"
        "} \n";

    Shader cubeMapShader(sourceVCubeMap, sourceFCubeMap);

    // Load objects
    char path[] = PATH_TO_OBJECTS "/sphere_smooth.obj";
    Object sphere1(path);
    sphere1.makeObject(shader);

    char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
    Object cubeMap(pathCube);
    cubeMap.makeObject(cubeMapShader);

    double prev = 0;
    int deltaFrame = 0;
    auto fps = [&](double now) {
        double deltaTime = now - prev;
        deltaFrame++;
        if (deltaTime > 0.5) {
            prev = now;
            const double fpsCount = (double)deltaFrame / deltaTime;
            deltaFrame = 0;
            std::cout << "\r FPS: " << fpsCount;
            std::cout.flush();
        }
    };

    glm::vec3 light_pos = glm::vec3(1.0, 2.0, 1.5);
    glm::mat4 model = glm::mat4(1.0);
    model = glm::translate(model, glm::vec3(0.0, 0.0, -2.0));
    model = glm::scale(model, glm::vec3(0.5, 0.5, 0.5));
    glm::mat4 inverseModel = glm::transpose(glm::inverse(model));

    // Create the cubemap texture
    GLuint cubeMapTexture;
    glGenTextures(1, &cubeMapTexture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::string pathToCubeMap = PATH_TO_TEXTURE "/cubemaps/yokohama3/";

    std::map<std::string, GLenum> facesToLoad = {
        {pathToCubeMap + "posx.jpg", GL_TEXTURE_CUBE_MAP_POSITIVE_X},
        {pathToCubeMap + "posy.jpg", GL_TEXTURE_CUBE_MAP_POSITIVE_Y},
        {pathToCubeMap + "posz.jpg", GL_TEXTURE_CUBE_MAP_POSITIVE_Z},
        {pathToCubeMap + "negx.jpg", GL_TEXTURE_CUBE_MAP_NEGATIVE_X},
        {pathToCubeMap + "negy.jpg", GL_TEXTURE_CUBE_MAP_NEGATIVE_Y},
        {pathToCubeMap + "negz.jpg", GL_TEXTURE_CUBE_MAP_NEGATIVE_Z},
    };
    
    for (std::pair<std::string, GLenum> pair : facesToLoad) {
        loadCubemapFace(pair.first.c_str(), pair.second);
    }

    // Ratio: Air (1.00) to Glass (1.52)
    float ratio = 1.00f / 1.52f;

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glm::mat4 view = camera.GetViewMatrix();
        
        // Take camera Zoom into account for projection
        glm::mat4 perspective = camera.GetProjectionMatrix();
        
        glfwPollEvents();
        double now = glfwGetTime();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

        // 1. Draw the Refractive Sphere
        shader.use();
        shader.setMatrix4("M", model);
        shader.setMatrix4("itM", inverseModel);
        shader.setMatrix4("V", view);
        shader.setMatrix4("P", perspective);
        shader.setVector3f("u_view_pos", camera.Position);

        // Send Refraction Ratio to shader
        shader.setFloat("refractionRatio", ratio);
        glUniform1i(glGetUniformLocation(shader.ID, "cubemapSampler"), 0);

        glDepthFunc(GL_LEQUAL);
        sphere1.draw();

        // 2. Draw the Skybox
        cubeMapShader.use();
        cubeMapShader.setMatrix4("V", view);
        cubeMapShader.setMatrix4("P", perspective);
        glUniform1i(glGetUniformLocation(cubeMapShader.ID, "cubemapSampler"), 0);

        cubeMap.draw();
        
        glDepthFunc(GL_LESS);

        fps(now);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Clean texture loader (Mac-friendly, removed stbi_failure_reason)
void loadCubemapFace(const char* path, const GLenum& targetFace)
{
    int imWidth, imHeight, imNrChannels;
    unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
    if (data) {
        GLenum format = (imNrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(targetFace, 0, format, imWidth, imHeight, 0, format, GL_UNSIGNED_BYTE, data);
    } else {
        std::cout << "Failed to Load texture at path: " << path << std::endl;
    }
    stbi_image_free(data);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

    // Movement WASD
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboardMovement(LEFT, 0.1);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboardMovement(RIGHT, 0.1);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboardMovement(FORWARD, 0.1);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboardMovement(BACKWARD, 0.1);
    
    // Camera Rotation
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) camera.ProcessKeyboardRotation(1, 0.0, 1);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) camera.ProcessKeyboardRotation(-1, 0.0, 1);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) camera.ProcessKeyboardRotation(0.0, 1.0, 1);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) camera.ProcessKeyboardRotation(0.0, -1.0, 1);

    // Zoom keys (Q and E)
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) camera.ProcessMouseScroll(0.5f);  // Zoom In
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) camera.ProcessMouseScroll(-0.5f); // Zoom Out
}