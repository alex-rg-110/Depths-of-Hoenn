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

// Mouse input setup
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

// Initialize camera slightly further back so the sphere is visible
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);

int main(int argc, char* argv[])
{
    if (!glfwInit()) throw std::runtime_error("Failed to initialise GLFW \n");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "Exercise 05 - Specular", nullptr, nullptr);
    if (window == NULL) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window\n");
    }

    glfwMakeContextCurrent(window);

    // Register mouse callback
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

    // Vertex Shader: Cleanly calculates Gouraud Specular Lighting
    const std::string sourceV = "#version 330 core\n"
        "in vec3 position; \n"
        "in vec2 tex_coords; \n"
        "in vec3 normal; \n"

        "out vec3 SpecularColor; \n"

        "uniform mat4 M; \n"
        "uniform mat4 itM; \n"
        "uniform mat4 V; \n"
        "uniform mat4 P; \n"
        
        "uniform vec3 u_light_pos; \n"
        "uniform vec3 u_view_pos; \n"

        // Light properties
        "const float shininess = 32.0; \n"
        "const float specularStrength = 0.8; \n"

        "void main(){ \n"
        // 1. Calculate World Position
        "   vec4 worldPos = M * vec4(position, 1.0); \n"
        "   gl_Position = P * V * worldPos; \n"
        
        // 2. Transform Normal
        "   vec3 worldNormal = normalize(vec3(itM * vec4(normal, 1.0))); \n" 
        
        // 3. Calculate Direction Vectors
        "   vec3 lightDir = normalize(u_light_pos - worldPos.xyz); \n"
        "   vec3 viewDir = normalize(u_view_pos - worldPos.xyz); \n"
        
        // 4. Calculate Reflection (GLSL reflect requires incidence vector, so we use -lightDir)
        "   vec3 reflectDir = reflect(-lightDir, worldNormal); \n"
        
        // 5. Specular Math (dot product of view and reflection)
        "   float specAmount = pow(max(dot(viewDir, reflectDir), 0.0), shininess); \n"
        
        // Output final specular color
        "   SpecularColor = vec3(specularStrength * specAmount); \n" 
        "}\n";

    // Fragment Shader: Just paints the interpolated specular highlight
    const std::string sourceF = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "precision mediump float; \n"

        "in vec3 SpecularColor; \n"

        "void main() { \n"      
        "   FragColor = vec4(SpecularColor, 1.0); \n"
        "} \n";


    Shader shader(sourceV, sourceF);

    // Using the macro path as requested
    char path[] = PATH_TO_OBJECTS "/sphere_smooth.obj";
    Object sphere1(path);
    sphere1.makeObject(shader);

    // Scene setup
    const glm::vec3 light_pos(1.0f, 2.0f, 2.0f);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.0f)); 
    model = glm::scale(model, glm::vec3(0.5f, 0.5f, 0.5f));
    
    glm::mat4 inverseModel = glm::inverseTranspose(model);

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
        
        // Dark grey background to clearly see the specular dot
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        
        // Send Matrices
        shader.setMatrix4("M", model);
        shader.setMatrix4("itM", inverseModel);
        shader.setMatrix4("V", view);
        shader.setMatrix4("P", perspective);
        
        // Send Positions using glUniform3fv for standard compatibility
        auto loc_view_pos = glGetUniformLocation(shader.ID, "u_view_pos");
        glUniform3fv(loc_view_pos, 1, glm::value_ptr(camera.Position));

        auto loc_light_pos = glGetUniformLocation(shader.ID, "u_light_pos");
        glUniform3fv(loc_light_pos, 1, glm::value_ptr(light_pos));

        // Draw Object
        sphere1.draw();

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
    float yoffset = lastY - ypos; // Invert y offset

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}