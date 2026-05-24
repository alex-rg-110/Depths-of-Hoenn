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
void APIENTRY glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const char* message, const void* userParam) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;
    std::cout << "Debug message (" << id << "): " << message << std::endl;
}
#endif

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);

int main(int argc, char* argv[])
{
    std::cout << "Welcome to exercice 7: Complete light equation and attenuation" << std::endl;

    if (!glfwInit()) throw std::runtime_error("Failed to initialise GLFW \n");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

    GLFWwindow* window = glfwCreateWindow(width, height, "Exercise 07", nullptr, nullptr);
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

    // Vertex Shader
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

    // Fragment Shader: Full Phong + Attenuation
    const std::string sourceF = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "precision mediump float; \n"

        "in vec3 v_frag_coord; \n"
        "in vec3 v_normal; \n"

        "uniform vec3 u_view_pos; \n"
        "uniform vec3 u_light_pos; \n"
        "uniform vec3 materialColour; \n"
        
        "uniform float ambientStrength; \n"
        "uniform float diffuseStrength; \n"
        "uniform float specularStrength; \n"

        "void main() { \n"
        "   vec3 N = normalize(v_normal); \n"
        "   vec3 L = normalize(u_light_pos - v_frag_coord); \n"
        "   vec3 ViewDir = normalize(u_view_pos - v_frag_coord); \n"
        "   vec3 R = reflect(-L, N); \n"
        
        // 1. Ambient
        "   vec3 ambient = ambientStrength * vec3(1.0); \n"
        
        // 2. Diffuse
        "   float diff = max(dot(N, L), 0.0); \n"
        "   vec3 diffuse = diffuseStrength * diff * vec3(1.0); \n"
        
        // 3. Specular
        "   float spec = pow(max(dot(ViewDir, R), 0.0), 32.0); \n"
        "   vec3 specular = specularStrength * spec * vec3(1.0); \n"
        
        // 4. Attenuation
        "   float distance = length(u_light_pos - v_frag_coord); \n"
        // Standard attenuation constants: Kc=1.0, Kl=0.09, Kq=0.032
        "   float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance)); \n"
        
        // Apply attenuation only to diffuse and specular
        "   diffuse *= attenuation; \n"
        "   specular *= attenuation; \n"
        
        // Final combination
        "   vec3 final_light = ambient + diffuse + specular; \n"
        "   FragColor = vec4(materialColour * final_light, 1.0); \n"
        "} \n";

    Shader shader(sourceV, sourceF);

    char path[] = PATH_TO_OBJECTS "/sphere_smooth.obj";
    Object sphere1(path);
    sphere1.makeObject(shader, false);

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

    // Light property parameters
    float ambient = 0.1f;
    float diffuse = 0.5f;
    float specular = 0.8f;
    glm::vec3 materialColour = glm::vec3(0.5f, 0.6f, 0.8f);

    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 perspective = camera.GetProjectionMatrix();
        glfwPollEvents();
        
        double now = glfwGetTime();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark background to see light fading better
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // Standard Matrices
        shader.setMatrix4("M", model);
        shader.setMatrix4("itM", inverseModel);
        shader.setMatrix4("V", view);
        shader.setMatrix4("P", perspective);
        
        // View position
        auto loc_view_pos = glGetUniformLocation(shader.ID, "u_view_pos");
        glUniform3fv(loc_view_pos, 1, glm::value_ptr(camera.Position));

        // Material Color
        auto loc_mat_col = glGetUniformLocation(shader.ID, "materialColour");
        glUniform3fv(loc_mat_col, 1, glm::value_ptr(materialColour));

        // Send Light Strengths
        glUniform1f(glGetUniformLocation(shader.ID, "ambientStrength"), ambient);
        glUniform1f(glGetUniformLocation(shader.ID, "diffuseStrength"), diffuse);
        glUniform1f(glGetUniformLocation(shader.ID, "specularStrength"), specular);
        
        // Calculate dynamic light position (moving closer and further in Z)
        auto delta = light_pos + glm::vec3(0.0f, 0.0f, 2.0f * std::sin(now));
        
        // Send the moving light position to the shader
        auto loc_light_pos = glGetUniformLocation(shader.ID, "u_light_pos");
        glUniform3fv(loc_light_pos, 1, glm::value_ptr(delta));

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
    float yoffset = lastY - ypos; 

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}