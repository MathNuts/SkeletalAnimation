#ifndef SCENE_HPP
#define SCENE_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

enum NodeType
{
	ROOT,
	LIGHT,
	GEOMETRY,
	GEOMETRY_2D,
	GEOMETRY_NM,
	CHARACTER,
};

struct Node
{
	std::vector<Node*> children;

	// The node's position and rotation relative to its parent
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	// A transformation matrix representing the transformation of the node's location relative to its parent. This matrix is updated every frame.
	glm::mat4 currentTransformationMatrix;

	// The location of the node's reference point
	glm::vec3 referencePoint;

	// The ID of the VAO containing the "appearance" of this SceneNode.
	std::vector<int> vertexArrayObjectIDs;
	std::vector<unsigned int> VAOIndexCounts;

	// Node type is used to determine how to handle the contents of a node
	NodeType type;
	int lightID;

	// Texture related IDs
	std::vector<unsigned int> textureIDs;
	std::vector<unsigned int> normalMapIDs;
	std::vector<unsigned int> specularMapIDs;

	Node()
	{
		type = GEOMETRY;
		position = glm::vec3(0, 0, 0);
		rotation = glm::vec3(0, 0, 0);
		scale = glm::vec3(1, 1, 1);
		referencePoint = glm::vec3(0, 0, 0);
	}
};

Node* createSceneNode()
{
	return new Node();
}

// Add a child node to its parent's list of children
void addChild(Node* parent, Node* child)
{
	parent->children.push_back(child);
}

#endif