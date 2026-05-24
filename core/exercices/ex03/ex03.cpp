#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "../../camera.h"
#include "../../shader.h"
#include "../object.h"

const int width = 500;
const int height = 500;

GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);

// Mouse input variables
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn);
float lastX = width / 2.0f;
float lastY = height / 2.0f;
bool firstMouse = true;

#ifndef NDEBUG
// Debug function 
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    std::cout << "Debug message (" << id << "): " << message << std::endl;
}
#endif

Camera camera(glm::vec3(1.0, 0.0, -6.0), glm::vec3(0.0, 1.0, 0.0), 90.0);

int main(int argc, char* argv[])
{
    if (!glfwInit()) throw std::runtime_error("Failed to initialise GLFW \n");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "Exercise 03", nullptr, nullptr);
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

    // Vertex Shader: Calculates Gouraud diffuse lighting
    const std::string sourceV = "#version 330 core\n"
        "in vec3 position; \n"
        "in vec2 tex_coord; \n"
        "in vec3 normal; \n"
        
        "out vec3 v_diffuse; \n"
        
        "uniform mat4 M; \n"
        "uniform mat4 V; \n"
        "uniform mat4 P; \n"
        
        // Professor's approach: Use mat4 instead of mat3
        "uniform mat4 itM; \n" 
        "uniform vec3 light_pos; \n"
        "uniform vec3 light_color; \n"

        " void main(){ \n"
        "   vec4 frag_coord = M * vec4(position, 1.0); \n"
        "   gl_Position = P * V * frag_coord;\n"
        
        // Professor's approach: multiply by vec4 then cast to vec3
        "   vec3 n = normalize(vec3(itM * vec4(normal, 1.0))); \n"
        
        // Calculate light direction
        "   vec3 l = normalize(light_pos - vec3(frag_coord)); \n"
        
        // Compute Diffuse using Lambert's cosine law
        "   float diff = max(dot(n, l), 0.0); \n"
        
        // Add a tiny bit of ambient light
        "   vec3 ambient = vec3(0.1); \n"
        "   v_diffuse = ambient + (diff * light_color);\n"
        "}\n"; 
        
    // Fragment Shader
    const std::string sourceF = "#version 330 core\n"
        "out vec4 FragColor;"
        "precision mediump float; \n"
        "in vec3 v_diffuse; \n"
        "void main() { \n"
        "   FragColor = vec4(v_diffuse, 1.0); \n"
        "} \n";

    Shader shader(sourceV, sourceF);

    // Load the model
    Object sphere("/Users/alejandrorodriguezgarcia/Desktop/UGent/3D graphics in VR/Exercises/mysphere.obj");
    sphere.makeObject(shader);

    // Light settings
    const glm::vec3 light_pos(2.0f, 2.0f, 2.0f);
    const glm::vec3 light_color(1.0f, 0.8f, 0.6f); 

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
        }
    };

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 perspective = camera.GetProjectionMatrix();
        glfwPollEvents();
        
        double now = glfwGetTime();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        glm::mat4 model = glm::mat4(1.0f); 
        
        // Professor's approach: Calculate Inverse Transpose as a mat4
        glm::mat4 itM = glm::inverseTranspose(model);

        shader.setMatrix4("M", model);
        shader.setMatrix4("V", view);
        shader.setMatrix4("P", perspective);
        
        // Professor's approach: Use setMatrix4
        shader.setMatrix4("itM", itM);
        
        auto loc_light_pos = glGetUniformLocation(shader.ID, "light_pos");
        glUniform3fv(loc_light_pos, 1, glm::value_ptr(light_pos));
        
        auto loc_light_col = glGetUniformLocation(shader.ID, "light_color");
        glUniform3fv(loc_light_col, 1, glm::value_ptr(light_color));

        sphere.draw();

        fps(now);
        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
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