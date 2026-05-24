#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

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
void loadCubemapFace(const char * file, const GLenum& targetCube);

// Mouse input variables
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
float lastX = width / 2.0f;
float lastY = height / 2.0f;
bool firstMouse = true;

#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    std::cout << "Debug message (" << id << "): " << message << std::endl;
}
#endif

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

int main(int argc, char* argv[])
{
    std::cout << "Welcome to exercice 8: Load a cubemap/skybox" << std::endl;

    if (!glfwInit()) throw std::runtime_error("Failed to initialise GLFW \n");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "Exercise 08", nullptr, nullptr);
    if (window == NULL) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window\n");
    }

    glfwMakeContextCurrent(window);

    // Setup mouse
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) throw std::runtime_error("Failed to initialize GLAD");

    glEnable(GL_DEPTH_TEST);

#ifndef NDEBUG
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(glDebugOutput, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
#endif

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

    const std::string sourceF = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "precision mediump float; \n"
        "in vec3 v_frag_coord; \n"
        "in vec3 v_normal; \n"
        "uniform vec3 u_view_pos; \n"
        "struct Light{\n" 
        "   vec3 light_pos; \n"
        "   float ambient_strength; \n"
        "   float diffuse_strength; \n"
        "   float specular_strength; \n"
        "   float constant;\n"
        "   float linear;\n"
        "   float quadratic;\n"
        "};\n"
        "uniform Light light;\n"
        "uniform float shininess; \n"
        "uniform vec3 materialColour; \n"
        "float specularCalculation(vec3 N, vec3 L, vec3 V ){ \n"
        "   vec3 R = reflect(-L, N); \n" 
        "   float cosTheta = dot(R, V); \n"
        "   float spec = pow(max(cosTheta, 0.0), 32.0); \n"
        "   return light.specular_strength * spec;\n"
        "}\n"
        "void main() { \n"
        "   vec3 N = normalize(v_normal);\n"
        "   vec3 L = normalize(light.light_pos - v_frag_coord); \n"
        "   vec3 V = normalize(u_view_pos - v_frag_coord); \n"
        "   float specular = specularCalculation(N, L, V); \n"
        "   float diffuse = light.diffuse_strength * max(dot(N, L), 0.0);\n"
        "   float distance = length(light.light_pos - v_frag_coord);\n"
        "   float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);\n"
        "   float total_light = light.ambient_strength + attenuation * (diffuse + specular); \n"
        "   FragColor = vec4(materialColour * vec3(total_light), 1.0); \n"
        "} \n";

    Shader shader(sourceV, sourceF);

    // Completed Vertex Shader for Skybox
    const std::string sourceVCubeMap = "#version 330 core\n"
        "in vec3 position; \n"
        "out vec3 texCoord_v; \n"
        "uniform mat4 V; \n"
        "uniform mat4 P; \n"
        "void main(){ \n"
        "   texCoord_v = position; \n"
        // Remove translation info from view matrix
        "   mat4 V_no_rot = mat4(mat3(V)); \n"
        "   vec4 pos = P * V_no_rot * vec4(position, 1.0); \n"
        // Force z to w, so z/w = 1.0 (Maximum depth)
        "   gl_Position = pos.xyww; \n"
        "}\n";

    // Completed Fragment Shader for Skybox
    const std::string sourceFCubeMap = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "precision mediump float; \n"
        "uniform samplerCube cubemapSampler; \n"
        "in vec3 texCoord_v; \n"
        "void main() { \n"
        "   FragColor = texture(cubemapSampler, texCoord_v); \n"
        "} \n";

    // Compile the shader for the cube map
    Shader cubemapShader(sourceVCubeMap, sourceFCubeMap);

    char path[] = PATH_TO_OBJECTS "/sphere_smooth.obj";
    Object sphere1(path);
    sphere1.makeObject(shader, false);
    
    // Load the cube model
    char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
    Object skyboxCube(pathCube);
    skyboxCube.makeObject(cubemapShader, false);

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

    glm::vec3 light_pos = glm::vec3(1.0f, 2.0f, 1.5f);
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.0f));
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    glm::mat4 inverseModel = glm::inverseTranspose(model);

    float ambient = 0.1f;
    float diffuse = 0.5f;
    float specular = 0.8f;
    glm::vec3 materialColour = glm::vec3(0.5f, 0.6f, 0.8f);

    shader.use();
    shader.setFloat("shininess", 32.0f);
    shader.setVector3f("materialColour", materialColour);
    shader.setFloat("light.ambient_strength", ambient);
    shader.setFloat("light.diffuse_strength", diffuse);
    shader.setFloat("light.specular_strength", specular);
    shader.setFloat("light.constant", 1.0f);
    shader.setFloat("light.linear", 0.14f);
    shader.setFloat("light.quadratic", 0.07f);

    // Create the cubemap texture
    GLuint cubeMapTexture;
    glGenTextures(1, &cubeMapTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

    // Set the texture parameters
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load the 6 images
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

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 perspective = camera.GetProjectionMatrix();
        glfwPollEvents();
        double now = glfwGetTime();
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. Draw the Sphere (Normal objects first)
        shader.use();
        shader.setMatrix4("M", model);
        shader.setMatrix4("itM", inverseModel);
        shader.setMatrix4("V", view);
        shader.setMatrix4("P", perspective);
        shader.setVector3f("u_view_pos", camera.Position);

        auto delta = light_pos + glm::vec3(0.0f, 0.0f, 2.0f * std::sin(now));
        shader.setVector3f("light.light_pos", delta);
        
        sphere1.draw();

        // 2. Draw the Skybox last for better performance
        glDepthFunc(GL_LEQUAL); // Important: change depth function to LEQUAL
        
        cubemapShader.use();
        cubemapShader.setMatrix4("V", view);
        cubemapShader.setMatrix4("P", perspective);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
        skyboxCube.draw();
        
        glDepthFunc(GL_LESS); // Set depth function back to default

        fps(now);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

// Completed load function
void loadCubemapFace(const char * path, const GLenum& targetFace)
{
    int imWidth, imHeight, imNrChannels;
    unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
    
    if (data) {
        // Find if the image has an alpha channel
        GLenum format = (imNrChannels == 4) ? GL_RGBA : GL_RGB;
        // Send the image to the buffer
        glTexImage2D(targetFace, 0, format, imWidth, imHeight, 0, format, GL_UNSIGNED_BYTE, data);
        // Free memory
        stbi_image_free(data);
    } else {
        std::cout << "Failed to Load texture at path: " << path << std::endl;
        const char* reason = stbi_failure_reason();
        std::cout << (reason == NULL ? "Unknown error" : reason) << std::endl;
    }
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboardMovement(LEFT, 0.1);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboardMovement(RIGHT, 0.1);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboardMovement(FORWARD, 0.1);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboardMovement(BACKWARD, 0.1);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}