#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include "shader.hpp"
#include "vaoutils.hpp"
#include "scene.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "helper.hpp"
#include "animation.hpp"
#include "animator.hpp"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window, Animation* animations);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void renderNode(Node* node);
void updateNodeTransformations(Node* node, glm::mat4 transformationThusFar);
void setUniformBoneTransforms(std::vector<glm::mat4> transforms, unsigned int shaderId);

bool VSYNC = true;
bool FULLSCREEN = false;
int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;
int FPS = 999999;

glm::vec3 cameraPos = glm::vec3(1.0f, 2.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool firstMouse = true;
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
float fov = 45.0f;

float deltaTime = 0.0f;

Animator animator = Animator();

Node* checkerFloor = createSceneNode();
Node* character = createSceneNode();

int main()
{
	// Init GLFW
	glfwInit();
	// Define OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	// Define usage of Core
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// For AA
	glfwWindowHint(GLFW_SAMPLES, 4);

	// Create window with GLFW
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Project", FULLSCREEN ? glfwGetPrimaryMonitor() : NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	// Create context
	glfwMakeContextCurrent(window);

	// Mouse callback
	glfwSetCursorPosCallback(window, mouse_callback);

	// Set opengl viewport same as window
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Disable mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Check if glad managed to load opengl, and only then continue using gl functions
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// Print info
	printInfo();

	// Depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Enable face culling for performance
	glEnable(GL_CULL_FACE);

	// Disable built-in dithering
	glDisable(GL_DITHER);

	// Enable transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!VSYNC)
		glfwSwapInterval(0);


	string daeFile = "../res/aj/aj.dae";
	//string daeFile = "../res/DanceMoves2.FBX";
	//string daeFile = "../res/vampire/dancing_vampire.dae";
	//string daeFile = "../res/person/model.dae";
	//string daeFile = "../res/maven/Maven.dae";

	string animFile1 = "../res/aj/breathing_idle.dae";
	string animFile2 = "../res/aj/walking.dae";
	string animFile3 = "../res/aj/right_strafe_walking.dae";
	string animFile4 = "../res/aj/left_strafe_walking.dae";
	string animFile5 = "../res/aj/walking_backwards.dae";
	string animFile6 = "../res/aj/jump.dae";

	vector<TextureOverride> overrides = {
		{ 0, DIFFUSE, "textures/Boy01_diffuse.jpg" },
		{ 0, NORMAL, "textures/Boy01_normal.jpg" },
		{ 0, SPECULAR, "textures/Boy01_spec.jpg" }
	};


	Model m = Model(daeFile, overrides);
	vector<Mesh> squareMeshes = m.meshes;
	std::cout << "Loaded meshes: " << m.meshes.size() << std::endl;

	Node* root = createSceneNode();
	root->type = ROOT;

	Mesh floorMesh;
	floorMesh.vertices = {
		glm::vec3(-20.0f, 0.0f, -20.0f),
		glm::vec3(-20.0f, 0.0f, 20.0f),
		glm::vec3(20.0f, 0.0f, 20.0f),
		glm::vec3(20.0f, 0.0f, -20.0f),
	};
	floorMesh.normals = {
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
	};
	floorMesh.indices = { 0, 1, 2, 2, 3, 0 };

	unsigned int floorVAO = generateBuffer(floorMesh);

	checkerFloor->type = GEOMETRY;
	checkerFloor->vertexArrayObjectIDs = { (int)floorVAO };
	checkerFloor->VAOIndexCounts = { (unsigned int)floorMesh.indices.size() };
	addChild(root, checkerFloor);



	character->type = CHARACTER;
	character->scale = glm::vec3(0.01, 0.01, 0.01);
	//character->scale = glm::vec3(0.1, 0.1, 0.1);
	//character->rotation = glm::vec3(-3.14 / 2.0, 0.0, 0.0);

	for (int i = 0; i < m.meshes.size(); i++)
	{
		unsigned int charVAO = generateBuffer(squareMeshes[i]);
		character->vertexArrayObjectIDs.push_back(charVAO);
		character->VAOIndexCounts.push_back(squareMeshes[i].indices.size());

		character->textureIDs.push_back(m.diffuseMaps[i]);
		character->normalMapIDs.push_back(m.normalMaps[i]);
		character->specularMapIDs.push_back(m.specularMaps[i]);
	}

	addChild(root, character);

	Animation anim1(animFile1, &m);
	Animation anim2(animFile2, &m);
	Animation anim3(animFile3, &m);
	Animation anim4(animFile4, &m);
	Animation anim5(animFile5, &m);
	Animation anim6(animFile6, &m);

	Animation animations[] = { anim1, anim2, anim3, anim4, anim5, anim6 };

	Shader shader = Shader("../src/shaders/default.vert", "../src/shaders/default.frag");
	Shader depthShader = Shader("../src/shaders/depth.vert", "../src/shaders/depth.frag");

	// Render loop
	float frameTime = 1.0f / FPS;
	float lastFrame = 0.0f;

	unsigned int depthMap;
	unsigned int depthFBO;

	const unsigned int s_width = 4096, s_height = 4096;

	generateDepthMap(depthMap, depthFBO, s_width, s_height);

	std::cout << "Starting.." << std::endl;

	while (!glfwWindowShouldClose(window))
	{
		float now = glfwGetTime();
		if (!VSYNC && now - lastFrame <= frameTime)
		{
			continue;
		}
		deltaTime = now - lastFrame;
		std::cout << "FPS: " << (1.0f / deltaTime) << "\t\r" << std::flush;
		lastFrame = now;

		processInput(window, animations);
		animator.updateAnimation(deltaTime);

		updateNodeTransformations(root, glm::mat4(1.0));

		auto transforms = animator.getFinalBoneMatrices();



		// ----------------- Shadow ---------------
		glCullFace(GL_FRONT);
		glm::mat4 lightProjection = glm::perspective(glm::radians(fov), (float)s_width / (float)s_height, 0.1f, 100.0f);
		glm::vec3 lightPos = glm::vec3(0.0f, 10.0f, 20.0f);
		glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
		glm::mat4 lightSpaceMatrix = lightProjection * lightView;

		depthShader.use();

		setUniformBoneTransforms(transforms, depthShader.ID);

		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

		glClearColor(0.5f, 1.0f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, s_width, s_height);
		glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		renderNode(root);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glCullFace(GL_BACK);

		shader.use();

		// ---------------- Shadow End ------------

		setUniformBoneTransforms(transforms, shader.ID);

		glClearColor(0.5f, 1.0f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

		glm::mat4 projection = glm::perspective(glm::radians(fov), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(cameraPos, character->position + glm::vec3(0.0f, 1.0f, 0.0f), cameraUp); // cameraPos + cameraFront

		glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(projection));
		glUniform3fv(3, 1, glm::value_ptr(cameraPos));

		glUniformMatrix4fv(5, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMap);

		renderNode(root);

		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	std::cout << std::endl
		<< "Terminating.."
		<< std::endl;

	// Terminate GLFW if we are done rendering
	glfwTerminate();
	return 0;
}

void setUniformBoneTransforms(std::vector<glm::mat4> transforms, unsigned int shaderId) {
	for (int i = 0; i < transforms.size(); ++i) {
		string boneStr = "boneTransforms[" + std::to_string(i) + "]";
		int boneLocation = glGetUniformLocation(shaderId, boneStr.c_str());
		glUniformMatrix4fv(boneLocation, 1, GL_FALSE, glm::value_ptr(transforms[i]));
	}
}

void updateNodeTransformations(Node* node, glm::mat4 transformationThusFar)
{
	glm::mat4 transformationMatrix =
		glm::translate(node->position) *
		glm::translate(node->referencePoint) *
		glm::rotate(node->rotation.y, glm::vec3(0, 1, 0)) *
		glm::rotate(node->rotation.x, glm::vec3(1, 0, 0)) *
		glm::rotate(node->rotation.z, glm::vec3(0, 0, 1)) *
		glm::scale(node->scale) *
		glm::translate(-node->referencePoint);

	node->currentTransformationMatrix = transformationThusFar * transformationMatrix;

	for (Node* child : node->children)
	{
		updateNodeTransformations(child, node->currentTransformationMatrix);
	}
}

void renderNode(Node* node)
{
	glUniform1ui(4, node->type);
	switch (node->type)
	{
	case CHARACTER:
		for (unsigned int i = 0; i < node->VAOIndexCounts.size(); i++)
			if (node->vertexArrayObjectIDs[i] != -1)
			{
				if (node->textureIDs[i] >= 0) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, node->textureIDs[i]);
				}

				if (node->normalMapIDs[i] >= 0) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, node->normalMapIDs[i]);
				}

				if (node->specularMapIDs[i] >= 0) {
					glActiveTexture(GL_TEXTURE2);
					glBindTexture(GL_TEXTURE_2D, node->specularMapIDs[i]);
				}

				glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
				glBindVertexArray(node->vertexArrayObjectIDs[i]);
				glDrawElements(GL_TRIANGLES, node->VAOIndexCounts[i], GL_UNSIGNED_INT, nullptr);
			}
		break;
	case GEOMETRY:
		for (unsigned int i = 0; i < node->VAOIndexCounts.size(); i++) {
			glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(node->currentTransformationMatrix));
			glBindVertexArray(node->vertexArrayObjectIDs[i]);
			glDrawElements(GL_TRIANGLES, node->VAOIndexCounts[i], GL_UNSIGNED_INT, nullptr);
		}
		break;
	}

	for (Node* child : node->children)
	{
		renderNode(child);
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, Animation* animations)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS)
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	float speed = 2.0f * deltaTime;

	bool idle = true;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		character->position.z += 0.75f * speed;
		cameraPos.z += 0.75f * speed;
		animator.playAnimation(&animations[1]);
		idle = false;
		//cameraPos += glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z)) * speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		character->position.z -= 0.5f * speed;
		cameraPos.z -= 0.5f * speed;
		animator.playAnimation(&animations[4]);
		idle = false;
		//cameraPos -= glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z)) * speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		character->position.x += 0.75f * speed;
		cameraPos.x += 0.75f * speed;
		animator.playAnimation(&animations[3]);
		idle = false;
		//cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		character->position.x -= 0.75f * speed;
		cameraPos.x -= 0.75f * speed;
		animator.playAnimation(&animations[2]);
		idle = false;
		//cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		animator.playAnimation(&animations[5]);
		idle = false;
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	if (idle) {
		animator.playAnimation(&animations[0]);
	}
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(direction);
}