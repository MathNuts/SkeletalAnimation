#ifndef VAOUTILS_HPP
#define VAOUTILS_HPP

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

#include "mesh.hpp"

void computeTangentBasis(
	std::vector<glm::vec3>& vertices,
	std::vector<glm::vec2>& uvs,
	std::vector<glm::vec3>& normals,
	std::vector<glm::vec3>& tangents,
	std::vector<glm::vec3>& bitangents)
{
	for (size_t i = 0; i < vertices.size() - 2; i += 3)
	{

		// Shortcuts for vertices
		glm::vec3& v0 = vertices[i + 0];
		glm::vec3& v1 = vertices[i + 1];
		glm::vec3& v2 = vertices[i + 2];

		// Shortcuts for UVs
		glm::vec2& uv0 = uvs[i + 0];
		glm::vec2& uv1 = uvs[i + 1];
		glm::vec2& uv2 = uvs[i + 2];

		// Edges of the triangle : position delta
		glm::vec3 deltaPos1 = v1 - v0;
		glm::vec3 deltaPos2 = v2 - v0;

		// UV delta
		glm::vec2 deltaUV1 = uv1 - uv0;
		glm::vec2 deltaUV2 = uv2 - uv0;

		float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
		glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
		glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

		// Set the same tangent for all three vertices of the triangle.
		// They will be merged later, in vboindexer.cpp
		tangents.push_back(tangent);
		tangents.push_back(tangent);
		tangents.push_back(tangent);

		// Same thing for bitangents
		bitangents.push_back(bitangent);
		bitangents.push_back(bitangent);
		bitangents.push_back(bitangent);
	}
}

template <class T>
unsigned int generateAttribute(int id, int elementsPerEntry, std::vector<T> data, bool normalize, bool integer = false)
{
	unsigned int bufferID;
	glGenBuffers(1, &bufferID);
	glBindBuffer(GL_ARRAY_BUFFER, bufferID);
	glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
	if (integer)
		glVertexAttribIPointer(id, elementsPerEntry, GL_INT, sizeof(T), 0);
	else
		glVertexAttribPointer(id, elementsPerEntry, GL_FLOAT, normalize ? GL_TRUE : GL_FALSE, sizeof(T), 0);
	glEnableVertexAttribArray(id);
	return bufferID;
}

unsigned int generateBuffer(Mesh& mesh)
{
	unsigned int vaoID;
	glGenVertexArrays(1, &vaoID);
	glBindVertexArray(vaoID);

	generateAttribute(0, 3, mesh.vertices, false);
	if (mesh.normals.size() > 0)
	{
		generateAttribute(1, 3, mesh.normals, true);
	}
	if (mesh.textureCoordinates.size() > 0)
	{
		generateAttribute(2, 2, mesh.textureCoordinates, false);

		// Init tangents
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec3> bitangents;

		if (mesh.tangents.size() == 0)
		{
			// Compute tangents
			computeTangentBasis(mesh.vertices, mesh.textureCoordinates, mesh.normals, tangents, bitangents);
		}
		else
		{
			// Use loaded tangets/bitangents
			tangents = mesh.tangents;
			bitangents = mesh.bitangents;
		}
		// Add tangents to VAO
		generateAttribute(3, 3, tangents, false);
		generateAttribute(4, 3, bitangents, false);
	}
	if (mesh.boneIDs.size() > 0)
	{
		generateAttribute(5, 4, mesh.boneIDs, false, true);
		generateAttribute(6, 4, mesh.weights, false);
	}

	unsigned int indexBufferID;
	glGenBuffers(1, &indexBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(unsigned int), mesh.indices.data(), GL_STATIC_DRAW);

	return vaoID;
}

void generateDepthMap(unsigned int& depthMap, unsigned int& FBO, unsigned int width, unsigned int height) {
	glGenFramebuffers(1, &FBO);

	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

#endif