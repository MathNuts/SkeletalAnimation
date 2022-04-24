#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "mesh.hpp"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

struct BoneProps
{
	string name;
	glm::mat4 offset;
};

enum TextureType { DIFFUSE, NORMAL, SPECULAR, HEIGHT };

struct TextureOverride
{
	unsigned int meshIndex;
	TextureType type;
	string path;
};

unsigned int textureFromFile(const char* path, const string& directory, bool gamma);
static glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from);

class Model
{
public:
	vector<Mesh> meshes;
	string directory;
	bool gammaCorrection;
	vector<TextureOverride> overrides;

	vector<unsigned int> diffuseMaps;
	vector<unsigned int> specularMaps;
	vector<unsigned int> normalMaps;
	vector<unsigned int> heightMaps;

	std::vector<BoneProps> boneProps;

	int boneCounter = 0;

	Model(string path, vector<TextureOverride> texOver, bool gamma = false) : overrides(texOver), gammaCorrection(gamma)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
			return;
		}

		directory = path.substr(0, path.find_last_of('/'));

		processNode(scene->mRootNode, scene);
	}

private:

	void extractBoneWeightForVertices(vector<glm::ivec4>& boneIDs_all, vector<glm::vec4>& weights_all, aiMesh* mesh, const aiScene* scene)
	{
		// Set the max bones to 100
		unsigned int numBones = mesh->mNumBones > 100 ? 100 : mesh->mNumBones;
		// For each bone
		for (unsigned int boneIndex = 0; boneIndex < numBones; ++boneIndex)
		{
			int boneID = -1;
			std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
			if (boneIndex >= boneProps.size()) {
				boneProps.push_back({ boneName, aiMatrix4x4ToGlm(&mesh->mBones[boneIndex]->mOffsetMatrix) });
				boneID = boneIndex;
				boneCounter++;
			}
			else {
				for (unsigned int i = 0; i < boneProps.size(); i++) {
					if (boneProps[i].name == boneName) {
						boneID = i;
						break;
					}
				}
			}
			assert(boneID != -1);

			// Get all vertex weights for current bone
			aiVertexWeight* weights = mesh->mBones[boneIndex]->mWeights;
			unsigned int numWeights = mesh->mBones[boneIndex]->mNumWeights;

			// For each weight at vertex x for current bone
			for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
			{
				unsigned int vertexId = weights[weightIndex].mVertexId;
				float weight = weights[weightIndex].mWeight;
				assert(vertexId <= boneIDs_all.size());

				// Update four most influential bones
				for (int i = 0; i < 4; ++i)
				{
					if (boneIDs_all[vertexId][i] < 0)
					{
						weights_all[vertexId][i] = weight;
						boneIDs_all[vertexId][i] = boneID;
						break;
					}
				}
			}
		}
	}

	void processNode(aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			meshes.push_back(processMesh(scene->mMeshes[node->mMeshes[i]], scene));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		// Mesh to fill with data
		Mesh m;

		// Loop all vertices in loaded mesh
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			glm::ivec4 boneIDs;
			glm::vec4 weights;

			// Set default values
			for (int i = 0; i < 4; i++)
			{
				boneIDs[i] = -1;
				weights[i] = 0.0f;
			}

			m.boneIDs.push_back(boneIDs);
			m.weights.push_back(weights);

			glm::vec3 vector;
			// Set positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			m.vertices.push_back(vector);

			if (mesh->HasNormals())
			{
				// Set normals
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				m.normals.push_back(vector);
			}

			if (mesh->mTextureCoords[0])
			{
				// Set texture coords
				glm::vec2 vec;
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				m.textureCoordinates.push_back(vec);
				if (mesh->HasTangentsAndBitangents()) {
					// Set tangent
					vector.x = mesh->mTangents[i].x;
					vector.y = mesh->mTangents[i].y;
					vector.z = mesh->mTangents[i].z;
					m.tangents.push_back(vector);
					// Set bitangent
					vector.x = mesh->mBitangents[i].x;
					vector.y = mesh->mBitangents[i].y;
					vector.z = mesh->mBitangents[i].z;
					m.bitangents.push_back(vector);
				}
			}
		}
		// Set indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				m.indices.push_back(face.mIndices[j]);
		}

		// Load mesh materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];


		// Any manual overrides?
		bool overrideDiffuse = false;
		bool overrideNormal = false;
		bool overrideSpecular = false;
		for (unsigned int i = 0; i < overrides.size(); i++) {
			if (overrides[i].meshIndex == meshes.size()) {
				if (overrides[i].type == DIFFUSE) {
					diffuseMaps.push_back(loadCustomTexture(overrides[i].path));
					overrideDiffuse = true;
				}
				else if (overrides[i].type == NORMAL) {
					normalMaps.push_back(loadCustomTexture(overrides[i].path));
					overrideNormal = true;
				}
				else if (overrides[i].type == SPECULAR) {
					specularMaps.push_back(loadCustomTexture(overrides[i].path));
					overrideSpecular = true;
				}
			}
		}

		// 1. diffuse maps
		if (!overrideDiffuse)
			diffuseMaps.push_back(loadMaterialTextures(material, aiTextureType_DIFFUSE));
		// 2. specular maps
		if (!overrideSpecular)
			specularMaps.push_back(loadMaterialTextures(material, aiTextureType_SPECULAR));
		// 3. normal maps
		if (!overrideNormal)
			normalMaps.push_back(loadMaterialTextures(material, aiTextureType_HEIGHT));
		// 4. height maps
		heightMaps.push_back(loadMaterialTextures(material, aiTextureType_AMBIENT));

		// Load boneIDs and weights for each vertex
		extractBoneWeightForVertices(m.boneIDs, m.weights, mesh, scene);

		cout << "Processed " << mesh->mNumBones << " bones, triangle count: " << m.boneIDs.size() << endl;

		return m;
	}

	unsigned int loadMaterialTextures(aiMaterial* mat, aiTextureType type)
	{
		unsigned int id = -1;
		for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			cout << "Loaded texture: " << str.C_Str() << endl;
			id = textureFromFile(str.C_Str(), this->directory, false);

			// Break after 1 texture of every type
			break;
		}
		return id;
	}

	unsigned int loadCustomTexture(string path)
	{
		cout << "Loaded custom texture: " << path.c_str() << endl;
		return textureFromFile(path.c_str(), this->directory, false);
	}
};

unsigned int textureFromFile(const char* path, const string& directory, bool gamma)
{
	string filename = string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RGB;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

static glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4* from)
{
	glm::mat4 to;
	to[0][0] = from->a1; to[1][0] = from->a2; to[2][0] = from->a3; to[3][0] = from->a4;
	to[0][1] = from->b1; to[1][1] = from->b2; to[2][1] = from->b3; to[3][1] = from->b4;
	to[0][2] = from->c1; to[1][2] = from->c2; to[2][2] = from->c3; to[3][2] = from->c4;
	to[0][3] = from->d1; to[1][3] = from->d2; to[2][3] = from->d3; to[3][3] = from->d4;
	return to;
}

#endif