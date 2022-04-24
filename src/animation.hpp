#ifndef ANIMATION_HPP
#define ANIMATION_HPP

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>

#include <string>
#include <vector>
#include <map>

#include "bone.hpp"
#include "model.hpp"

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};


class Animation
{
public:
	Animation(const std::string& animationPath, Model* model)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
		assert(scene && scene->mRootNode);
		if (scene->mNumAnimations == 0)
			return;
		aiAnimation* animation = scene->mAnimations[0];
		duration = (float)animation->mDuration;
		tps = (float)animation->mTicksPerSecond;
		generateBoneTree(&rootNode, scene->mRootNode);
		// Reset all root transformations
		rootNode.transformation = glm::mat4(1.0f);
		loadIntermediateBones(animation, model);
	}

	Bone* findBone(const std::string& name)
	{
		for (unsigned int i = 0; i < bones.size(); i++) {
			if (bones[i].getBoneName() == name) {
				return &bones[i];
			}
		}
		return nullptr;
	}


	inline float getTicksPerSecond() { return tps; }

	inline float getDuration() { return duration; }

	inline const AssimpNodeData* getRootNode() { return &rootNode; }

	inline const std::vector<BoneProps>& getBoneProps()
	{
		return boneProps;
	}

private:
	float duration = 0.0f;
	float tps = 0.0f;
	std::vector<Bone> bones;
	AssimpNodeData rootNode;
	std::vector<BoneProps> boneProps;

	void loadIntermediateBones(const aiAnimation* animation, Model* model)
	{
		auto& boneProps = model->boneProps;

		for (int i = 0; i < animation->mNumChannels; i++)
		{
			auto channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.data;
			int boneId = -1;

			for (unsigned int i = 0; i < boneProps.size(); i++) {
				if (boneProps[i].name == boneName) {
					boneId = i;
					break;
				}
			}

			if (boneProps.size() < 100) {
				if (boneId == -1) {
					BoneProps boneProp;
					boneProp.name = boneName;
					boneProps.push_back(boneProp);
					boneId = boneProps.size() - 1;
				}
			}
			bones.push_back(Bone(channel->mNodeName.data, boneId, channel));
		}

		this->boneProps = boneProps;
	}

	void generateBoneTree(AssimpNodeData* parent, const aiNode* src)
	{
		assert(src);

		parent->name = src->mName.data;
		parent->transformation = aiMatrix4x4ToGlm(&src->mTransformation);
		parent->childrenCount = src->mNumChildren;

		for (unsigned int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			generateBoneTree(&newData, src->mChildren[i]);
			parent->children.push_back(newData);
		}
	}
};

#endif