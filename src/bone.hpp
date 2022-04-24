#ifndef BONE_HPP
#define BONE_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/scene.h>

#include <vector>

#include "interpolation.hpp"

class Bone
{
private:
	glm::mat4 transform;
	std::vector<KeyPosition> positions;
	std::vector<KeyRotation> rotations;
	std::vector<KeyScale> scales;
	size_t numPositions;
	size_t numRotations;
	size_t numScalings;
	std::string name;
	unsigned int id;

public:
	Bone(const std::string& inName, int inId, const aiNodeAnim* channel) {
		name = inName;
		id = inId;
		transform = glm::mat4(1.0f);

		numPositions = channel->mNumPositionKeys;

		for (int positionIndex = 0; positionIndex < numPositions; ++positionIndex)
		{
			aiVector3D aiPosition = channel->mPositionKeys[positionIndex].mValue;
			float timeStamp = (float)channel->mPositionKeys[positionIndex].mTime;
			KeyPosition data = { glm::vec3(aiPosition.x, aiPosition.y, aiPosition.z), timeStamp };
			positions.push_back(data);
		}

		numRotations = channel->mNumRotationKeys;
		for (int rotationIndex = 0; rotationIndex < numRotations; ++rotationIndex)
		{
			aiQuaternion aiOrientation = channel->mRotationKeys[rotationIndex].mValue;
			float timeStamp = (float)channel->mRotationKeys[rotationIndex].mTime;
			KeyRotation data = { glm::quat(aiOrientation.w, aiOrientation.x, aiOrientation.y, aiOrientation.z), timeStamp };
			rotations.push_back(data);
		}

		numScalings = channel->mNumScalingKeys;
		for (int keyIndex = 0; keyIndex < numScalings; ++keyIndex)
		{
			aiVector3D scale = channel->mScalingKeys[keyIndex].mValue;
			float timeStamp = (float)channel->mScalingKeys[keyIndex].mTime;
			KeyScale data = { glm::vec3(scale.x, scale.y, scale.z), timeStamp };
			scales.push_back(data);
		}
	}

	KeyPosition getPositions(float animationTime) {
		size_t posIndex = (animationTime == 0.0f) ? 0 : getPositionIndex(animationTime) + 1;
		return positions[posIndex];
	}

	KeyRotation getRotations(float animationTime) {
		size_t rotIndex = (animationTime == 0.0f) ? 0 : getRotationIndex(animationTime) + 1;
		return rotations[rotIndex];
	}

	KeyScale getScalings(float animationTime) {
		size_t sclIndex = (animationTime == 0.0f) ? 0 : getScaleIndex(animationTime) + 1;
		return scales[sclIndex];
	}


	void update(float animationTime)
	{
		size_t posIndex = getPositionIndex(animationTime);
		glm::mat4 translation;
		if (numPositions == 1) {
			translation = glm::translate(glm::mat4(1.0f), positions[0].position);
		}
		else
			translation = interpolatePosition(animationTime, positions[posIndex], positions[posIndex + 1]);

		size_t rotIndex = getRotationIndex(animationTime);
		glm::mat4 rotation;
		if (numRotations == 1)
			rotation = glm::toMat4(glm::normalize(rotations[0].orientation));
		else
			rotation = interpolateRotation(animationTime, rotations[rotIndex], rotations[rotIndex + 1]);

		size_t sclIndex = getScaleIndex(animationTime);
		glm::mat4 scale;
		if (numScalings == 1)
			scale = glm::scale(glm::mat4(1.0f), scales[0].scale);
		else
			scale = interpolateScaling(animationTime, scales[sclIndex], scales[sclIndex + 1]);
		transform = translation * rotation * scale;
	}

	glm::mat4 getTransform() { return transform; }
	std::string getBoneName() const { return name; }
	unsigned int getId() const { return id; }

	size_t getPositionIndex(float animationTime)
	{
		for (size_t index = 0; index < numPositions - 1; ++index)
		{
			if (animationTime < positions[index + 1].timeStamp)
				return index;
		}
		return numPositions - 2;
	}

	size_t getRotationIndex(float animationTime)
	{
		for (size_t index = 0; index < numRotations - 1; ++index)
		{
			if (animationTime < rotations[index + 1].timeStamp)
				return index;
		}
		return numRotations - 2;
	}

	size_t getScaleIndex(float animationTime)
	{
		for (size_t index = 0; index < numScalings - 1; ++index)
		{
			if (animationTime < scales[index + 1].timeStamp)
				return index;
		}
		return numScalings - 2;
	}
};



#endif