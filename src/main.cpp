#include<iostream>

// Include glad before GLFW to avoid header conflict or define "#define GLFW_INCLUDE_NONE"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>


#include "stb_image.h"
#include <map>
#include <string>



#include "../core/camera.h"
#include "../core/shader.h"
#include "../core/solutions/object.h"
#include "model.h"
#include <vector>
#include <cstdlib>


const int width = 500;
const int height = 500;


GLuint compileShader(std::string shaderCode, GLenum shaderType);
GLuint compileProgram(GLuint vertexShader, GLuint fragmentShader);
void processInput(GLFWwindow* window);

void loadCubemapFace(const char * file, const GLenum& targetCube);


#ifndef NDEBUG
void APIENTRY glDebugOutput(GLenum source,
	GLenum type,
	unsigned int id,
	GLenum severity,
	GLsizei length,
	const char* message,
	const void* userParam)
{
	// Ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	std::cout << "---------------" << std::endl;
	std::cout << "Debug message (" << id << "): " << message << std::endl;

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
	case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
	} std::cout << std::endl;

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
	case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
	case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
	} std::cout << std::endl;

	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
	} std::cout << std::endl;
	std::cout << std::endl;
}
#endif

Camera camera(glm::vec3(0.0, 0.0, 0.0));

float lastX = 250.0f, lastY = 250.0f;
bool firstMouse = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessKeyboardRotation(xoffset, yoffset, 0.05f);
}


int main(int argc, char* argv[])
{
	std::cout << "Depths of Hoenn - Loading..." << std::endl;


	// Boilerplate
	// Create the OpenGL context 
	if (!glfwInit()) {
		throw std::runtime_error("Failed to initialise GLFW \n");
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifndef NDEBUG
	// Create a debug context to help with Debugging
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif


	// Create the window
	GLFWwindow* window = glfwCreateWindow(width, height, "Depths of Hoenn", nullptr, nullptr);
	if (window == NULL)
	{
		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window\n");
	}

	glfwMakeContextCurrent(window);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);



	//load openGL function
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		throw std::runtime_error("Failed to initialize GLAD");
	}

	glEnable(GL_DEPTH_TEST);

#ifndef NDEBUG
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(glDebugOutput, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}
#endif

	const std::string sourceV = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coord; \n"
		"in vec3 normal; \n"
		"out vec3 v_frag_coord; \n"
		"out vec3 v_normal; \n"
		"out vec2 v_tex_coord; \n"
		"uniform mat4 M; \n"
		"uniform mat4 itM; \n"
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		"void main(){ \n"
		"vec4 frag_coord = M*vec4(position, 1.0); \n"
		"gl_Position = P*V*frag_coord; \n"
		"v_normal = vec3(itM * vec4(normal, 1.0)); \n"
		"v_frag_coord = frag_coord.xyz; \n"
		"v_tex_coord = tex_coord; \n"
		"}\n";

	const std::string sourceF = "#version 400 core\n"
		"out vec4 FragColor;\n"
		"in vec3 v_frag_coord; \n"
		"in vec3 v_normal; \n"
		"in vec2 v_tex_coord; \n"
		"uniform vec3 u_view_pos; \n"
		"uniform vec3 lightPos; \n"
		"uniform vec3 lightPos2; \n"
		"uniform vec3 lightPos3; \n"
		"uniform sampler2D texture1; \n"
		"uniform bool useTexture; \n"
		"uniform vec3 objectColor; \n"
		"void main() { \n"
		"vec3 baseColor = useTexture ? texture(texture1, v_tex_coord).rgb : objectColor;\n"
		"vec3 N = normalize(v_normal);\n"
		// Toon: quantize diffuse from light 3 (main scene light)
		"vec3 L3 = normalize(lightPos3 - v_frag_coord);\n"
		"float diff3 = max(dot(N, L3), 0.0);\n"
		// Quantize into 4 steps
		"float toon = floor(diff3 * 4.0) / 4.0;\n"
		// Bioluminescent lights smooth for atmosphere
		"vec3 L1 = normalize(lightPos - v_frag_coord);\n"
		"float diff1 = max(dot(N, L1), 0.0);\n"
		"float dist1 = length(lightPos - v_frag_coord);\n"
		"float att1 = 1.0 / (1.0 + 0.09*dist1 + 0.032*dist1*dist1);\n"
		"vec3 bio1 = diff1 * att1 * vec3(0.0, 0.8, 0.9) * baseColor;\n"
		"vec3 L2 = normalize(lightPos2 - v_frag_coord);\n"
		"float diff2 = max(dot(N, L2), 0.0);\n"
		"float dist2 = length(lightPos2 - v_frag_coord);\n"
		"float att2 = 1.0 / (1.0 + 0.09*dist2 + 0.032*dist2*dist2);\n"
		"vec3 bio2 = diff2 * att2 * vec3(0.1, 0.4, 1.0) * baseColor;\n"
		// Ambient
		"vec3 ambient = 0.6 * baseColor;\n"
		// Toon shaded main light
		"vec3 toonLight = toon * vec3(0.9, 1.1, 1.3) * baseColor;\n"
		"vec3 lit = ambient + toonLight + bio1 + bio2;\n"
		// Depth fog (Beer Lambert)
		"float dist = length(u_view_pos - v_frag_coord);\n"
		"float fogDensity = 0.05;\n"
		"float fogFactor = exp(-fogDensity * dist);\n"
		"fogFactor = clamp(fogFactor, 0.0, 1.0);\n"
		"vec3 fogColor = vec3(0.0, 0.08, 0.18);\n"
		"vec3 finalColor = mix(fogColor, lit, fogFactor);\n"
		"FragColor = vec4(finalColor, 1.0);\n"
		"} \n";

	Shader shader(sourceV, sourceF);

	const std::string sourceVReflect = "#version 330 core\n"
    "in vec3 position;\n"
    "in vec3 normal;\n"
    "in vec2 tex_coord;\n"
    "out vec3 v_normal;\n"
    "out vec3 v_frag_coord;\n"
    "out vec2 v_tex_coord;\n"
    "uniform mat4 M;\n"
    "uniform mat4 itM;\n"
    "uniform mat4 V;\n"
    "uniform mat4 P;\n"
    "void main(){\n"
    "vec4 frag_coord = M * vec4(position, 1.0);\n"
    "gl_Position = P * V * frag_coord;\n"
    "v_frag_coord = frag_coord.xyz;\n"
    "v_normal = vec3(itM * vec4(normal, 1.0));\n"
    "v_tex_coord = tex_coord;\n"
    "}\n";

	const std::string sourceFReflect = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec3 v_normal;\n"
		"in vec3 v_frag_coord;\n"
		"in vec2 v_tex_coord;\n"
		"uniform vec3 u_view_pos;\n"
		"uniform samplerCube cubemapSampler;\n"
		"uniform sampler2D texture1;\n"
		"uniform bool useTexture;\n"
		"void main(){\n"
		"vec3 N = normalize(v_normal);\n"
		"vec3 V = normalize(u_view_pos - v_frag_coord);\n"
		"vec3 R = reflect(-V, N);\n"
		"vec3 T = refract(-V, N, 0.85);\n"
		"vec3 reflectColor = texture(cubemapSampler, R).rgb;\n"
		"vec3 refractColor = texture(cubemapSampler, T).rgb;\n"
		"vec3 baseColor = useTexture ? texture(texture1, v_tex_coord).rgb : vec3(0.1, 0.3, 0.8);\n"
		"vec3 finalColor = mix(baseColor, mix(reflectColor, refractColor, 0.4), 0.4);\n"
		"FragColor = vec4(finalColor, 1.0);\n"
		"}\n";

Shader reflectShader(sourceVReflect, sourceFReflect);

	const std::string sourceVCubeMap = "#version 330 core\n"
		"in vec3 position; \n"
		"in vec2 tex_coords; \n"
		"in vec3 normal; \n"
		// Only P and V are necessary
		"uniform mat4 V; \n"
		"uniform mat4 P; \n"
		"out vec3 texCoord_v; \n"
		" void main(){ \n"
		"texCoord_v = position;\n"
		// Remove translation info from view matrix to only keep rotation
		"mat4 V_no_rot = mat4(mat3(V)) ;\n"
		"vec4 pos = P * V_no_rot * vec4(position, 1.0); \n"
		// Positions xyz are divided by w after the vertex shader
		// z component is equal to the depth value
		// z always equal to 1.0 here, so we set z = w!
		"gl_Position = pos.xyww;\n"
		"\n" 
		"}\n";

	const std::string sourceVBubble = "#version 330 core\n"
		"in vec3 position;\n"
		"out vec2 v_uv;\n"
		"uniform mat4 V;\n"
		"uniform mat4 P;\n"
		"uniform vec3 particlePos;\n"
		"uniform float size;\n"
		"void main(){\n"
		"vec3 camRight = vec3(V[0][0], V[1][0], V[2][0]);\n"
		"vec3 camUp = vec3(V[0][1], V[1][1], V[2][1]);\n"
		"vec3 worldPos = particlePos + camRight * position.x * size + camUp * position.y * size;\n"
		"gl_Position = P * V * vec4(worldPos, 1.0);\n"
		"v_uv = position.xy + vec2(0.5);\n"
		"}\n";

	const std::string sourceFBubble = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec2 v_uv;\n"
		"uniform float alpha;\n"
		"void main(){\n"
		"float dist = length(v_uv - vec2(0.5));\n"
		"if (dist > 0.5) discard;\n"
		"float rim = smoothstep(0.3, 0.5, dist);\n"
		"FragColor = vec4(0.85, 0.97, 1.0, rim * alpha);\n"
		"}\n";

	Shader bubbleShader(sourceVBubble, sourceFBubble);

	const std::string sourceVOrb = "#version 330 core\n"
		"in vec3 position;\n"
		"out vec2 v_uv;\n"
		"uniform mat4 M;\n"
		"uniform mat4 V;\n"
		"uniform mat4 P;\n"
		"void main(){\n"
		"gl_Position = P * V * M * vec4(position, 1.0);\n"
		"v_uv = position.xy + vec2(0.5);\n"
		"}\n";

	const std::string sourceFOrb = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec2 v_uv;\n"
		"uniform float time;\n"
		"void main(){\n"
		"vec2 center = v_uv - vec2(0.5);\n"
		"float dist = length(center);\n"
		"if (dist > 0.5) discard;\n"
		// 3D sphere shading
		"vec3 normal = vec3(center * 2.0, sqrt(1.0 - clamp(dot(center*2.0, center*2.0), 0.0, 1.0)));\n"
		"normal = normalize(normal);\n"
		"vec3 lightDir = normalize(vec3(0.5, 1.0, 0.8));\n"
		"float diff = max(dot(normal, lightDir), 0.0);\n"
		"float pulse = 0.85 + 0.15 * sin(time * 2.0);\n"
		"float shimmer = 0.5 + 0.5 * sin(time * 3.0 + dist * 20.0);\n"
		"vec3 coreColor = vec3(0.2, 0.6, 1.0);\n"
		"vec3 brightColor = vec3(0.6, 0.9, 1.0);\n"
		// Specular highlight
		"vec3 viewDir = vec3(0.0, 0.0, 1.0);\n"
		"vec3 reflectDir = reflect(-lightDir, normal);\n"
		"float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);\n"
		"vec3 color = mix(coreColor, brightColor, diff) * pulse;\n"
		"color += vec3(1.0, 1.0, 1.0) * spec * 0.8;\n"
		"color += vec3(0.4, 0.7, 1.0) * shimmer * 0.3;\n"
		"float alpha = (0.7 + 0.3 * diff) * pulse;\n"
		"FragColor = vec4(color, alpha);\n"
		"}\n";

Shader orbShader(sourceVOrb, sourceFOrb);

	char pathSphere[] = PATH_TO_OBJECTS "/cube.obj";
	Object orbMesh(pathSphere);
	orbMesh.makeObject(orbShader);

	float quadVertices[] = {
		-0.5f, -0.5f, 0.0f,
		0.5f, -0.5f, 0.0f,
		0.5f,  0.5f, 0.0f,
		-0.5f, -0.5f, 0.0f,
		0.5f,  0.5f, 0.0f,
		-0.5f,  0.5f, 0.0f,
	};
	GLuint quadVAO, quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glBindVertexArray(0);

	struct Bubble {
		glm::vec3 pos;
		float speed;
		float size;
		float phase;
	};
	// Bubbles speed, size, and phase and quantity
	std::vector<Bubble> bubbles;
	for (int i = 0; i < 80; i++) {
		Bubble b;
		b.pos = glm::vec3(
			((rand() % 200) - 100) * 0.06f,
			-2.0f + (rand() % 100) * 0.04f,
			-4.0f - (rand() % 100) * 0.08f
	);
		b.speed = 0.4f + (rand() % 100) * 0.008f;
		b.size = 0.08f + (rand() % 100) * 0.003f;
		b.phase = (rand() % 100) * 0.1f;
		bubbles.push_back(b);
	}
	// Instance for chinchous
	const int NUM_CHINCHOU = 25;
		glm::vec3 chinchouOffsets[NUM_CHINCHOU];
		for (int i = 0; i < NUM_CHINCHOU; i++) {
			chinchouOffsets[i] = glm::vec3(
				((rand() % 100) - 50) * 0.04f,
				((rand() % 100) - 50) * 0.02f,
				((rand() % 100) - 50) * 0.04f
			);
		}

	// Blue Orbs
	const int NUM_ORBS = 5;
		glm::vec3 orbPositions[NUM_ORBS] = {
			glm::vec3(-7.0f, -1.2f, -14.0f),   // left reef — slightly adjusted
			glm::vec3(8.5f, -1.2f, -15.0f),    // right reef — slightly adjusted
			glm::vec3(-1.0f, -1.2f, -27.0f),   // deep center
			glm::vec3(5.5f, -1.2f, -24.0f),    // deep right
			glm::vec3(-5.5f, -1.2f, -24.0f),   // deep left
		};
		bool orbCollected[NUM_ORBS] = {false, false, false, false, false};
		int orbsCollected = 0;

		bool gameWon = false;
		float winTime = 0.0f;

	const std::string sourceFCubeMap = 
		"#version 330 core\n"
		"out vec4 FragColor;\n"
		"precision mediump float; \n"
		"in vec3 texCoord_v; \n"
		"void main() { \n"
		"vec3 deepOcean = vec3(0.0, 0.08, 0.18);\n"
		"vec3 midOcean = vec3(0.0, 0.15, 0.35);\n"
		"float t = clamp((texCoord_v.y + 1.0) / 2.0, 0.0, 1.0);\n"
		"FragColor = vec4(mix(deepOcean, midOcean, t), 1.0);\n"
		"} \n";


	Shader cubeMapShader = Shader(sourceVCubeMap, sourceFCubeMap);

	const std::string sourceVFBO = "#version 330 core\n"
    "in vec2 position;\n"
    "out vec2 v_uv;\n"
    "void main(){\n"
    "gl_Position = vec4(position, 0.0, 1.0);\n"
    "v_uv = position * 0.5 + 0.5;\n"
    "}\n";

	const std::string sourceFBO = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec2 v_uv;\n"
		"uniform sampler2D screenTexture;\n"
		"uniform float time;\n"
		"uniform int orbsCollected;\n"
		"uniform int gameWon;\n"
		"void main(){\n"
		// Underwater wave distortion
		"float waveX = sin(v_uv.y * 20.0 + time * 1.5) * 0.003;\n"
		"float waveY = sin(v_uv.x * 20.0 + time * 1.2) * 0.003;\n"
		"vec2 distortedUV = v_uv + vec2(waveX, waveY);\n"
		// Sample scene with distortion
		"vec3 color = texture(screenTexture, distortedUV).rgb;\n"
		// Slight blue tint at edges (vignette)
		"float vignette = 1.0 - length(v_uv - 0.5) * 1.2;\n"
		"vignette = clamp(vignette, 0.0, 1.0);\n"
		"color *= vignette;\n"
		// Subtle chromatic aberration
		"float r = texture(screenTexture, distortedUV + vec2(0.002, 0.0)).r;\n"
		"float b = texture(screenTexture, distortedUV - vec2(0.002, 0.0)).b;\n"
		"color.r = mix(color.r, r, 0.3);\n"
		"color.b = mix(color.b, b, 0.3);\n"
		"vec2 hudStart = vec2(0.05, 0.92);\n"
		"float orbSize = 0.025;\n"
		"float orbSpacing = 0.06;\n"
		"for (int i = 0; i < 5; i++) {\n"
		"  vec2 orbCenter = hudStart + vec2(float(i) * orbSpacing, 0.0);\n"
		"  float d = length(v_uv - orbCenter);\n"
		"  if (d < orbSize) {\n"
		"    vec3 orbColor = (i < orbsCollected) ? vec3(0.2,0.6,1.0) : vec3(0.1,0.1,0.3);\n"
		"    color = mix(color, orbColor, 0.9);\n"
		"  }\n"
		"}\n"
		"if (gameWon > 0) {\n"
		"  float rings = sin(length(v_uv - vec2(0.5)) * 40.0 - time * 6.0);\n"
		"  rings = smoothstep(0.3, 1.0, rings) * 0.15 * (1.0 - length(v_uv - vec2(0.5)) * 2.0);\n"
		"  color += vec3(0.0, 0.3, 0.8) * max(rings, 0.0);\n"
		"}\n"
		"FragColor = vec4(color, 1.0);\n"
		"}\n";

	Shader fboShader(sourceVFBO, sourceFBO);

	const std::string sourceVWin = "#version 330 core\n"
    "in vec2 position;\n"
    "void main(){\n"
    "gl_Position = vec4(position, 0.0, 1.0);\n"
    "}\n";

	const std::string sourceFWin = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"uniform float alpha;\n"
		"void main(){\n"
		"FragColor = vec4(0.0, 0.5, 0.8, alpha);\n"
		"}\n";

	Shader winShader(sourceVWin, sourceFWin);

	float screenQuad[] = {
		-1.0f, -1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
		-1.0f, -1.0f,
		 1.0f,  1.0f,
		-1.0f,  1.0f,
	};
	GLuint screenVAO, screenVBO;
	glGenVertexArrays(1, &screenVAO);
	glGenBuffers(1, &screenVBO);
	glBindVertexArray(screenVAO);
	glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuad), screenQuad, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glBindVertexArray(0);

	// Pokemons loading
	char path[] = PATH_TO_OBJECTS "/Kyogre/scene.gltf";
	Model sphere1(path);
	sphere1.makeObject(reflectShader);

	char pathMilotic[] = PATH_TO_OBJECTS "/milotic/scene.gltf";
	Model milotic(pathMilotic);
	milotic.makeObject(shader);

	char pathWailord[] = PATH_TO_OBJECTS "/wailord/scene.gltf";
	Model wailord(pathWailord);
	wailord.makeObject(shader);

	char pathRelicanth[] = PATH_TO_OBJECTS "/relicanth/scene.gltf";
	Model relicanth(pathRelicanth);
	relicanth.makeObject(shader);

	char pathLumineon[] = PATH_TO_OBJECTS "/lumineon/scene.gltf";
	Model lumineon(pathLumineon);
	lumineon.makeObject(shader);

	char pathChinchou[] = PATH_TO_OBJECTS "/chinchou/scene.gltf";
	Model chinchou(pathChinchou);
	chinchou.makeObject(shader);

	// Ocean floor
	char pathFloor[] = PATH_TO_OBJECTS "/plane.obj";
	Object oceanFloor(pathFloor);
	oceanFloor.makeObject(shader);
	glm::mat4 floorModel = glm::mat4(1.0);
	floorModel = glm::translate(floorModel, glm::vec3(0.0, -2.5, -5.0));
	floorModel = glm::scale(floorModel, glm::vec3(45.0, 1.5, 45.0));
	glm::mat4 floorInverse = glm::transpose(glm::inverse(floorModel));


	// Corals
	char pathCoral[] = PATH_TO_OBJECTS "/coral/scene.gltf";
	Model coral(pathCoral);
	coral.makeObject(shader);

	char pathCube[] = PATH_TO_OBJECTS "/cube.obj";
	Object cubeMap(pathCube);
	cubeMap.makeObject(cubeMapShader);
	

	double prev = 0;
	int deltaFrame = 0;
	//fps function
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

	// Kyogre
	glm::vec3 light_pos = glm::vec3(0.0, 3.0, -5.0);
	glm::mat4 model = glm::mat4(1.0);
	model = glm::translate(model, glm::vec3(0.0, 0.5, -5.0));
	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
	model = glm::scale(model, glm::vec3(0.01, 0.01, 0.01));
	glm::mat4 inverseModel = glm::transpose( glm::inverse(model));

	// Milotic 
	glm::mat4 modelMilotic = glm::mat4(1.0);
	modelMilotic = glm::rotate(modelMilotic, glm::radians(90.0f), glm::vec3(1.0, 0.0, 0.0));
	modelMilotic = glm::scale(modelMilotic, glm::vec3(0.75, 0.75, 0.75));

	// Wailord 
	glm::mat4 modelWailord = glm::mat4(1.0);
	modelWailord = glm::scale(modelWailord, glm::vec3(0.015, 0.015, 0.015));

	// Relicanth (near floor)
	glm::mat4 modelRelicanth = glm::mat4(1.0);
	modelRelicanth = glm::rotate(modelRelicanth, glm::radians(225.0f), glm::vec3(0.0, 1.0, 1.0));
	modelRelicanth = glm::scale(modelRelicanth, glm::vec3(0.008, 0.008, 0.008));

	// Lumineon 
	glm::mat4 modelLumineon = glm::mat4(1.0);
	modelLumineon = glm::scale(modelLumineon, glm::vec3(1.0, 1.0, 1.0));

	// Chinchou 
	glm::mat4 modelChinchou = glm::mat4(1.0);
	modelChinchou = glm::scale(modelChinchou, glm::vec3(0.75, 0.75, 0.75));
	glm::mat4 view = camera.GetViewMatrix();
	glm::mat4 perspective = camera.GetProjectionMatrix();

	float ambient = 0.1;
	float diffuse = 0.5;
	float specular = 0.8;

	glm::vec3 materialColour = glm::vec3(0.5f,0.6,0.8);

	//Rendering

	shader.use();
	shader.setFloat("shininess", 32.0f);
	shader.setVector3f("materialColour", materialColour);
	shader.setFloat("light.ambient_strength", ambient);
	shader.setFloat("light.diffuse_strength", diffuse);
	shader.setFloat("light.specular_strength", specular);
	shader.setFloat("light.constant", 1.0);
	shader.setFloat("light.linear", 0.14);
	shader.setFloat("light.quadratic", 0.07);


	

	GLuint cubeMapTexture;
	glGenTextures(1, &cubeMapTexture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);

	// Texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Cubemap with dark ocean color programmatically
	unsigned char facePixels[4*4*3];
	for (int i = 0; i < 6; i++) {
		for (int y = 0; y < 4; y++) {
			for (int x = 0; x < 4; x++) {
				int idx = (y*4+x)*3;
				if (i == 2) { // Top (bright caustic light from surface)
					facePixels[idx+0] = 20 + x*10;
					facePixels[idx+1] = 80 + y*10;
					facePixels[idx+2] = 120 + x*8;
				} else if (i == 3) { // Bottom (dark abyss)
					facePixels[idx+0] = 0;
					facePixels[idx+1] = 5;
					facePixels[idx+2] = 15;
				} else { // Sides (mid ocean blue with variation)
					facePixels[idx+0] = 0 + x*3;
					facePixels[idx+1] = 20 + y*5;
					facePixels[idx+2] = 60 + x*8;
				}
			}
		}
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 4, 4, 0, GL_RGB, GL_UNSIGNED_BYTE, facePixels);
	}

	// FBO setup
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	GLuint fboTexture;
	glGenTextures(1, &fboTexture);
	glBindTexture(GL_TEXTURE_2D, fboTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

	GLuint rbo;
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glfwSwapInterval(1);
	glm::vec3 lumineonPos = glm::vec3(-2.0f, 0.0f, -4.0f); // starting position
	const glm::vec3 coralPositions[] = {
		glm::vec3(2.0f,-2.0f,-5.0f), glm::vec3(-2.0f,-2.0f,-5.5f), glm::vec3(1.0f,-2.0f,-6.0f), glm::vec3(-1.0f,-2.0f,-4.5f), glm::vec3(3.0f,-2.0f,-5.5f),
		glm::vec3(-6.0f,-2.0f,-10.0f), glm::vec3(-8.0f,-2.0f,-12.0f), glm::vec3(-7.0f,-2.0f,-15.0f), glm::vec3(-5.0f,-2.0f,-18.0f), glm::vec3(-9.0f,-2.0f,-20.0f),
		glm::vec3(-6.0f,-2.0f,-13.0f), glm::vec3(-8.0f,-2.0f,-17.0f), glm::vec3(-10.0f,-2.0f,-15.0f),
		glm::vec3(6.0f,-2.0f,-10.0f), glm::vec3(8.0f,-2.0f,-13.0f), glm::vec3(7.0f,-2.0f,-16.0f), glm::vec3(5.0f,-2.0f,-19.0f), glm::vec3(9.0f,-2.0f,-22.0f),
		glm::vec3(6.0f,-2.0f,-14.0f), glm::vec3(8.0f,-2.0f,-18.0f), glm::vec3(10.0f,-2.0f,-16.0f),
		glm::vec3(0.0f,-2.0f,-14.0f), glm::vec3(2.0f,-2.0f,-18.0f), glm::vec3(-2.0f,-2.0f,-22.0f), glm::vec3(1.0f,-2.0f,-26.0f), glm::vec3(-1.0f,-2.0f,-30.0f),
		glm::vec3(3.0f,-2.0f,-22.0f), glm::vec3(-3.0f,-2.0f,-26.0f),
		glm::vec3(4.0f,-2.0f,-25.0f), glm::vec3(-4.0f,-2.0f,-28.0f), glm::vec3(10.0f,-2.0f,-25.0f), glm::vec3(-10.0f,-2.0f,-25.0f),
		glm::vec3(6.0f,-2.0f,-32.0f), glm::vec3(-6.0f,-2.0f,-32.0f), glm::vec3(3.0f,-2.0f,-32.0f), glm::vec3(-3.0f,-2.0f,-28.0f),
	};
	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		// Floor and ceiling collision
		if (camera.Position.y < -1.0f) camera.Position.y = -1.0f;
		if (camera.Position.y > 8.0f) camera.Position.y = 8.0f;
		// Coral collision for player
		for (int i = 0; i < 35; i++) {
			glm::vec3 diff = glm::vec3(camera.Position.x - coralPositions[i].x, 0.0f, camera.Position.z - coralPositions[i].z);
			float dist = glm::length(diff);
			if (dist < 1.5f && dist > 0.0f) {
				camera.Position.x += glm::normalize(diff).x * (1.5f - dist);
				camera.Position.z += glm::normalize(diff).z * (1.5f - dist);
			}
		}
		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && gameWon) {
			for (int i = 0; i < NUM_ORBS; i++) orbCollected[i] = false;
			orbsCollected = 0;
			gameWon = false;
		}
		view = camera.GetViewMatrix();
		glfwPollEvents();
		double now = glfwGetTime();
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glEnable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.08f, 0.18f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		shader.use();

		glm::mat4 animatedModel = glm::mat4(1.0f);
		if (gameWon) {
			float t = (float)now - winTime;
			float growProgress = glm::min(t / 5.0f, 1.0f);
			float eased = growProgress * growProgress * (3.0f - 2.0f * growProgress);
			float scale = glm::mix(0.025f, 0.035f, eased);
			float pulse = 1.0f + 0.05f * std::sin(now * 8.0f) * (1.0f - eased);
			if (t < 5.0f) {
				// Still in place growing
				animatedModel = glm::translate(animatedModel, glm::vec3(0.0f, 2.0f, 15.0f));
				animatedModel = glm::rotate(animatedModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				animatedModel = glm::rotate(animatedModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				animatedModel = glm::scale(animatedModel, glm::vec3(scale * pulse));
			} else {
				// Starts swimming 
				float swimT = t - 5.0f;
				float swimX = 5.0f * std::sin(swimT * 0.2f);
				float swimZ = 15.0f - swimT * 0.5f;
				float swimAngle = atan2(5.0f * 0.2f * std::cos(swimT * 0.2f), -0.5f);
				animatedModel = glm::translate(animatedModel, glm::vec3(swimX, 2.5f + 0.3f * std::sin(now * 0.8f), swimZ));
				animatedModel = glm::rotate(animatedModel, swimAngle, glm::vec3(0.0f, 1.0f, 0.0f));
				animatedModel = glm::rotate(animatedModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				animatedModel = glm::scale(animatedModel, glm::vec3(0.035f));
			}
		} else {
			// Dormant statue behind the player
			animatedModel = glm::translate(animatedModel, glm::vec3(0.0f, 2.0f, 15.0f));
			animatedModel = glm::rotate(animatedModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			animatedModel = glm::rotate(animatedModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			animatedModel = glm::scale(animatedModel, glm::vec3(0.025f));
		}

		shader.setMatrix4("M", animatedModel);
		glm::mat4 animatedInverse = glm::transpose(glm::inverse(animatedModel));
		shader.setMatrix4("itM", animatedInverse);
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_view_pos", camera.Position);


		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		cubeMapShader.setInteger("cubemapTexture", 0);

		reflectShader.use();
		reflectShader.setMatrix4("M", animatedModel);
		reflectShader.setMatrix4("itM", animatedInverse);
		reflectShader.setMatrix4("V", view);
		reflectShader.setMatrix4("P", perspective);
		reflectShader.setVector3f("u_view_pos", camera.Position);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapTexture);
		reflectShader.setInteger("cubemapSampler", 0);
		if (sphere1.hasTexture) {
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, sphere1.textureID);
			reflectShader.setInteger("texture1", 1);
			reflectShader.setInteger("useTexture", 1);
		} else {
			reflectShader.setInteger("useTexture", 0);
		}
		sphere1.draw();
		shader.use();
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setVector3f("u_view_pos", camera.Position);


		// Milotic movement
		float mX = 5.0f + 2.0f * std::sin(now * 0.3f);
		float mZ = -8.0f + 3.0f * std::sin(now * 0.15f);
		for (int ci = 0; ci < 35; ci++) {
			glm::vec2 diff2 = glm::vec2(mX - coralPositions[ci].x, mZ - coralPositions[ci].z);
			float d2 = glm::length(diff2);
			if (d2 < 1.5f && d2 > 0.0f) {
				mX += glm::normalize(diff2).x * (1.5f - d2);
				mZ += glm::normalize(diff2).y * (1.5f - d2);
			}
		}
		float mY = 0.5f + 0.3f * std::sin(now * 0.4f);
		if (mY < -1.5f) mY = -1.5f;
		// Face swimming direction
		float mXNext = 5.0f + 2.0f * std::sin((now + 0.05) * 0.3f);
		float mZNext = -8.0f + 3.0f * std::sin((now + 0.05) * 0.15f);
		glm::vec3 mDir = glm::normalize(glm::vec3(mXNext - mX, 0.0f, mZNext - mZ));
		float mAngle = atan2(mDir.x, mDir.z);
		glm::mat4 animMilotic = glm::translate(glm::mat4(1.0f), glm::vec3(mX, mY, mZ));
		animMilotic = glm::rotate(animMilotic, mAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		animMilotic = glm::rotate(animMilotic, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		animMilotic = glm::scale(animMilotic, glm::vec3(0.75f));
		shader.setMatrix4("M", animMilotic);
		shader.setMatrix4("itM", glm::transpose(glm::inverse(animMilotic)));
		if (milotic.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, milotic.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
		else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.8, 0.5, 0.7)); }
		milotic.draw();

		// Wailord 1 movement
		float wX = -8.0f + 4.0f * std::sin(now * 0.08f);
		float wZ = -18.0f + 2.0f * std::cos(now * 0.08f);
		for (int ci = 0; ci < 35; ci++) {
			glm::vec2 diff2 = glm::vec2(mX - coralPositions[ci].x, mZ - coralPositions[ci].z);
			float d2 = glm::length(diff2);
			if (d2 < 1.5f && d2 > 0.0f) {
				mX += glm::normalize(diff2).x * (1.5f - d2);
				mZ += glm::normalize(diff2).y * (1.5f - d2);
			}
		}
		float wY = 1.5f + 0.3f * std::sin(now * 0.12f);
		if (wY < -1.5f) wY = -1.5f;
		float wXNext = -3.0f + 4.0f * std::sin((now + 0.05) * 0.08f);
		float wZNext = -20.0f + 2.0f * std::cos((now + 0.05) * 0.08f);
		glm::vec3 wDir = glm::normalize(glm::vec3(wXNext - wX, 0.0f, wZNext - wZ));
		float wAngle = atan2(wDir.x, wDir.z);
		glm::mat4 animWailord = glm::translate(glm::mat4(1.0f), glm::vec3(wX, wY, wZ));
		animWailord = glm::rotate(animWailord, wAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		animWailord = glm::scale(animWailord, glm::vec3(0.015f));
		shader.setMatrix4("M", animWailord);
		shader.setMatrix4("itM", glm::transpose(glm::inverse(animWailord)));
		if (wailord.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, wailord.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
		else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.3, 0.4, 0.8)); }
		wailord.draw();

		// Wailord 2 movement
		float w2X = 12.0f + 3.0f * std::sin(now * 0.06f);
		float w2Z = -32.0f + 2.0f * std::cos(now * 0.07f);
		for (int ci = 0; ci < 35; ci++) {
			glm::vec2 diff2 = glm::vec2(mX - coralPositions[ci].x, mZ - coralPositions[ci].z);
			float d2 = glm::length(diff2);
			if (d2 < 1.5f && d2 > 0.0f) {
				mX += glm::normalize(diff2).x * (1.5f - d2);
				mZ += glm::normalize(diff2).y * (1.5f - d2);
			}
		}
		float w2Y = 3.0f + 0.4f * std::sin(now * 0.1f);
		if (w2Y < -1.5f) w2Y = -1.5f;
		float w2XNext = 8.0f + 3.0f * std::sin((now + 0.05) * 0.06f);
		float w2ZNext = -25.0f + 2.0f * std::cos((now + 0.05) * 0.07f);
		glm::vec3 w2Dir = glm::normalize(glm::vec3(w2XNext - w2X, 0.0f, w2ZNext - w2Z));
		float w2Angle = atan2(w2Dir.x, w2Dir.z);
		glm::mat4 animWailord2 = glm::translate(glm::mat4(1.0f), glm::vec3(w2X, w2Y, w2Z));
		animWailord2 = glm::rotate(animWailord2, w2Angle, glm::vec3(0.0f, 1.0f, 0.0f));
		animWailord2 = glm::scale(animWailord2, glm::vec3(0.012f));
		shader.setMatrix4("M", animWailord2);
		shader.setMatrix4("itM", glm::transpose(glm::inverse(animWailord2)));
		if (wailord.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, wailord.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
		else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.3, 0.4, 0.8)); }
		wailord.draw();

		// Relicanth movement near sea floor
		float rX = 4.0f + 2.5f * std::sin(now * 0.2f);
		float rZ = -7.0f + 2.0f * std::cos(now * 0.2f);
		glm::mat4 animRelicanth = glm::mat4(1.0f);
		animRelicanth = glm::translate(animRelicanth, glm::vec3(rX, -1.8f, rZ));
		animRelicanth = glm::rotate(animRelicanth, glm::radians(225.0f), glm::vec3(0.0f, 1.0f, 1.0f));
		animRelicanth = glm::scale(animRelicanth, glm::vec3(0.008f));
		shader.setMatrix4("M", animRelicanth);
		shader.setMatrix4("itM", glm::transpose(glm::inverse(animRelicanth)));
		if (relicanth.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, relicanth.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
		else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.4, 0.5, 0.3)); }
		relicanth.draw();

		// Relicanth 2 movement near sea floor
		float r2X = -4.0f + 2.0f * std::sin(now * 0.15f);
		float r2Z = -10.0f + 2.5f * std::cos(now * 0.15f);
		glm::mat4 animRelicanth2 = glm::mat4(1.0f);
		animRelicanth2 = glm::translate(animRelicanth2, glm::vec3(r2X, -1.8f, r2Z));
		animRelicanth2 = glm::rotate(animRelicanth2, glm::radians(225.0f), glm::vec3(0.0f, 1.0f, 1.0f));
		animRelicanth2 = glm::scale(animRelicanth2, glm::vec3(0.008f));
		shader.setMatrix4("M", animRelicanth2);
		shader.setMatrix4("itM", glm::transpose(glm::inverse(animRelicanth2)));
		if (relicanth.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, relicanth.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
		else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.4, 0.5, 0.3)); }
		relicanth.draw();

		// Lumineon guides player toward nearest uncollected orb
		float closestDist = 9999.0f;
		glm::vec3 closestOrb = camera.Position;
		for (int i = 0; i < NUM_ORBS; i++) {
			if (!orbCollected[i]) {
				float d = glm::length(camera.Position - orbPositions[i]);
				if (d < closestDist) {
					closestDist = d;
					closestOrb = orbPositions[i];
				}
			}
		}
		float lerpSpeed = 0.015f;
		lumineonPos = glm::mix(lumineonPos, closestOrb + glm::vec3(0.0f, 1.5f, 0.0f), lerpSpeed);
		float lX = lumineonPos.x + 0.3f * std::sin(now * 1.2f);
		float lZ = lumineonPos.z + 0.3f * std::cos(now * 0.9f);
		float lY = lumineonPos.y + 0.2f * std::sin(now * 1.5f);

		glm::mat4 animLumineon = glm::translate(modelLumineon, glm::vec3(lX, lY, lZ));
		shader.setMatrix4("M", animLumineon);
		shader.setMatrix4("itM", glm::transpose(glm::inverse(animLumineon)));
		if (lumineon.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, lumineon.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
		else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.2, 0.7, 0.9)); }
		lumineon.draw();

		// Chinchou school instanced
		float cX = 2.0f + 2.0f * std::sin(now * 0.5f);
		float cZ = -8.0f + 2.0f * std::cos(now * 0.5f);
		for (int ci = 0; ci < 35; ci++) {
			glm::vec2 diff2 = glm::vec2(mX - coralPositions[ci].x, mZ - coralPositions[ci].z);
			float d2 = glm::length(diff2);
			if (d2 < 1.5f && d2 > 0.0f) {
				mX += glm::normalize(diff2).x * (1.5f - d2);
				mZ += glm::normalize(diff2).y * (1.5f - d2);
			}
		}
		float cY = -1.2f + 0.15f * std::sin(now * 0.8f);
		if (cY < -1.5f) cY = -1.5f;
		float cXNext = 2.0f + 2.0f * std::sin((now + 0.05) * 0.5f);
		float cZNext = -8.0f + 2.0f * std::cos((now + 0.05) * 0.5f);
		glm::vec3 cDir = glm::normalize(glm::vec3(cXNext - cX, 0.0f, cZNext - cZ));
		float cAngle = atan2(cDir.x, cDir.z);
		// Scatter from player
		float scatterDist = glm::length(glm::vec2(camera.Position.x - cX, camera.Position.z - cZ));
		if (scatterDist < 3.0f) {
			glm::vec2 scatterDir = glm::normalize(glm::vec2(cX - camera.Position.x, cZ - camera.Position.z));
			cX += scatterDir.x * (3.0f - scatterDist) * 0.08f;
			cZ += scatterDir.y * (3.0f - scatterDist) * 0.08f;
		}
		for (int i = 0; i < NUM_CHINCHOU; i++) {
			glm::mat4 animChinchouI = glm::translate(glm::mat4(1.0f), glm::vec3(cX + chinchouOffsets[i].x, cY + chinchouOffsets[i].y, cZ + chinchouOffsets[i].z));
			animChinchouI = glm::rotate(animChinchouI, cAngle, glm::vec3(0.0f, 1.0f, 0.0f));
			animChinchouI = glm::scale(animChinchouI, glm::vec3(0.3f));
			shader.setMatrix4("M", animChinchouI);
			shader.setMatrix4("itM", glm::transpose(glm::inverse(animChinchouI)));
			if (chinchou.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, chinchou.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
			else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.3, 0.5, 0.9)); }
			chinchou.draw();
		}

		// Chinchou school 2 
		float c2X = -4.0f + 2.0f * std::sin(now * 0.4f);
		float c2Z = -12.0f + 2.0f * std::cos(now * 0.4f);
		for (int ci = 0; ci < 35; ci++) {
			glm::vec2 diff2 = glm::vec2(c2X - coralPositions[ci].x, c2Z - coralPositions[ci].z);
			float d2 = glm::length(diff2);
			if (d2 < 1.5f && d2 > 0.0f) {
				c2X += glm::normalize(diff2).x * (1.5f - d2);
				c2Z += glm::normalize(diff2).y * (1.5f - d2);
			}
		}
		float c2Y = -1.0f + 0.15f * std::sin(now * 0.9f);
		if (c2Y < -1.5f) c2Y = -1.5f;
		float c2XNext = -4.0f + 2.0f * std::sin((now + 0.05) * 0.4f);
		float c2ZNext = -12.0f + 2.0f * std::cos((now + 0.05) * 0.4f);
		glm::vec3 c2Dir = glm::normalize(glm::vec3(c2XNext - c2X, 0.0f, c2ZNext - c2Z));
		float c2Angle = atan2(c2Dir.x, c2Dir.z);
		// Scatter from player — school 2
		float scatterDist2 = glm::length(glm::vec2(camera.Position.x - c2X, camera.Position.z - c2Z));
		if (scatterDist2 < 3.0f) {
			glm::vec2 scatterDir2 = glm::normalize(glm::vec2(c2X - camera.Position.x, c2Z - camera.Position.z));
			c2X += scatterDir2.x * (3.0f - scatterDist2) * 0.08f;
			c2Z += scatterDir2.y * (3.0f - scatterDist2) * 0.08f;
		}
		for (int i = 0; i < NUM_CHINCHOU; i++) {
			glm::mat4 animChinchou2 = glm::translate(glm::mat4(1.0f), glm::vec3(c2X + chinchouOffsets[i].x, c2Y + chinchouOffsets[i].y, c2Z + chinchouOffsets[i].z));
			animChinchou2 = glm::rotate(animChinchou2, c2Angle, glm::vec3(0.0f, 1.0f, 0.0f));
			animChinchou2 = glm::scale(animChinchou2, glm::vec3(0.3f));
			shader.setMatrix4("M", animChinchou2);
			shader.setMatrix4("itM", glm::transpose(glm::inverse(animChinchou2)));
			if (chinchou.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, chinchou.textureID); shader.setInteger("texture1", 1); shader.setInteger("useTexture", 1); }
			else { shader.setInteger("useTexture", 0); shader.setVector3f("objectColor", glm::vec3(0.3, 0.5, 0.9)); }
			chinchou.draw();
		}

		shader.setVector3f("lightPos", glm::vec3(lX, lY, lZ));
		shader.setVector3f("lightPos2", glm::vec3(cX, cY, cZ));
		shader.setVector3f("lightPos3", light_pos);
	

		shader.use();
		shader.setMatrix4("V", view);
		shader.setMatrix4("P", perspective);
		shader.setInteger("useTexture", 0);
		shader.setVector3f("objectColor", glm::vec3(0.0, 0.55, 0.65));
		shader.setVector3f("lightPos", glm::vec3(lX, lY, lZ));
		shader.setVector3f("lightPos2", glm::vec3(cX, cY, cZ));
		shader.setVector3f("lightPos3", light_pos);
	

		// Ocean floor
		shader.setMatrix4("M", floorModel);
		shader.setMatrix4("itM", floorInverse);
		oceanFloor.draw();	

		// Coral reef
		float coralScales[] = {
			0.4f,0.3f,0.5f,0.35f,0.45f,
			0.4f,0.3f,0.5f,0.35f,0.45f,0.4f,0.3f,0.5f,
			0.4f,0.35f,0.45f,0.3f,0.5f,0.4f,0.35f,0.45f,
			0.4f,0.35f,0.45f,0.3f,0.5f,0.4f,0.35f,
			0.45f,0.4f,0.35f,0.3f,0.5f,0.4f,0.35f
		};
		float coralRotations[] = {
			0.0f,45.0f,90.0f,135.0f,180.0f,
			30.0f,60.0f,120.0f,150.0f,200.0f,240.0f,270.0f,300.0f,
			15.0f,75.0f,210.0f,330.0f,45.0f,90.0f,180.0f,135.0f,
			270.0f,60.0f,330.0f,15.0f,75.0f,210.0f,330.0f,
			45.0f,90.0f,180.0f,135.0f,270.0f,60.0f,330.0f
		};
		for (int i = 0; i < 35; i++) {
			glm::mat4 m = glm::translate(glm::mat4(1.0f), coralPositions[i]);
			m = glm::rotate(m, glm::radians(coralRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
			m = glm::scale(m, glm::vec3(coralScales[i]));
			shader.setMatrix4("M", m);
			shader.setMatrix4("itM", glm::transpose(glm::inverse(m)));
			shader.setInteger("useTexture", coral.hasTexture ? 1 : 0);
			if (coral.hasTexture) { glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, coral.textureID); shader.setInteger("texture1", 1); }
			else shader.setVector3f("objectColor", glm::vec3(0.6f, 0.2f, 0.7f));
			coral.draw();
		}


		// Blue Orb collection check
		glm::vec3 playerPos = camera.Position;
		for (int i = 0; i < NUM_ORBS; i++) {
			if (!orbCollected[i]) {
				float dist = glm::length(playerPos - orbPositions[i]);
				if (dist < 1.5f) {
					orbCollected[i] = true;
					orbsCollected++;
					std::cout << "\n🔵 Blue Orb collected! " << orbsCollected << "/" << NUM_ORBS << std::endl;
					if (orbsCollected == NUM_ORBS) {
						std::cout << "\n✨ All Blue Orbs collected! Kyogre awakens!" << std::endl;
						gameWon = true;
						winTime = (float)now;
					}
				}
			}
		}

		// Draw orbs
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		orbShader.use();
		orbShader.setMatrix4("V", view);
		orbShader.setMatrix4("P", perspective);
		orbShader.setFloat("time", (float)now);
		for (int i = 0; i < NUM_ORBS; i++) {
			if (!orbCollected[i]) {
				glm::mat4 orbModel = glm::mat4(1.0f);
				orbModel = glm::translate(orbModel, orbPositions[i]);
				orbModel = glm::scale(orbModel, glm::vec3(0.3f));
				orbShader.setMatrix4("M", orbModel);
				orbMesh.draw();
			}
		}
		glDisable(GL_BLEND);

		// Bubbles
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		bubbleShader.use();
		bubbleShader.setMatrix4("V", view);
		bubbleShader.setMatrix4("P", perspective);
		for (auto& b : bubbles) {
			float y = b.pos.y + std::fmod(now * b.speed + b.phase, 5.0f);
			float alpha = 0.8f;
			if (y > 2.0f) alpha = 1.0f - (y - 2.0f) / 1.5f;
			if (alpha < 0.0f) alpha = 0.0f;
			float wobble = 0.1f * std::sin(now * 2.0f + b.phase);
			glm::vec3 drawPos = glm::vec3(b.pos.x + wobble, y, b.pos.z);
			bubbleShader.setVector3f("particlePos", drawPos);
			bubbleShader.setFloat("size", b.size);
			bubbleShader.setFloat("alpha", alpha * 0.85f);
			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		glDisable(GL_BLEND);

		glDepthFunc(GL_LEQUAL);
		cubeMapShader.use();
		cubeMapShader.setMatrix4("V", view);
		cubeMapShader.setMatrix4("P", perspective);
		cubeMapShader.setInteger("cubemapTexture", 0);

		cubeMap.draw();
		glDepthFunc(GL_LESS);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		fboShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fboTexture);
		fboShader.setInteger("screenTexture", 0);
		fboShader.setFloat("time", (float)now);
		fboShader.setInteger("orbsCollected", orbsCollected);
		fboShader.setInteger("gameWon", gameWon ? 1 : 0);
		glBindVertexArray(screenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		if (gameWon) {
			float t = glm::min((float)now - winTime, 3.0f);
			float alpha = t / 3.0f;
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			winShader.use();
			winShader.setFloat("alpha", alpha * 0.6f);
			glBindVertexArray(screenVAO);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			glDisable(GL_BLEND);
			if (alpha >= 1.0f)
				std::cout << "\r✨ KYOGRE AWAKENS! Press R to restart or ESC to quit" << std::flush;
		}
		glEnable(GL_DEPTH_TEST);


		fps(now);
		glfwSwapBuffers(window);
	}

	// Clean up ressource
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void loadCubemapFace(const char * path, const GLenum& targetFace)
{
	int imWidth, imHeight, imNrChannels;
	unsigned char* data = stbi_load(path, &imWidth, &imHeight, &imNrChannels, 0);
	if (data)
	{

		glTexImage2D(targetFace, 0, GL_RGB, imWidth, imHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else {
		std::cout << "Failed to Load texture" << std::endl;
		const char* reason = stbi_failure_reason();
		std::cout << reason << std::endl;
	}
	stbi_image_free(data);
}


void processInput(GLFWwindow* window) {
	// Use the cameras class to change the parameters of the camera
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(LEFT, 0.2);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(RIGHT, 0.2);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(FORWARD, 0.2);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboardMovement(BACKWARD, 0.2);

}

