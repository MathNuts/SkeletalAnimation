#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include "animation.hpp"

class Animator
{
private:
	std::vector<glm::mat4> finalBoneMatrices;
	Animation* currentAnimation;
	Animation* nextAnimation;
	Animation* queueAnimation;
	float currentTime;
	bool interpolating;
	float haltTime;
	float interTime;

public:
	Animator()
	{
		currentTime = 0.0;
		interpolating = false;
		haltTime = 0.0;
		interTime = 0.0;

		currentAnimation = nullptr;
		nextAnimation = nullptr;
		queueAnimation = nullptr;

		finalBoneMatrices.reserve(100);

		for (int i = 0; i < 100; i++)
			finalBoneMatrices.push_back(glm::mat4(1.0f));
	}

	void updateAnimation(float dt)
	{
		if (currentAnimation) {
			currentTime = fmod(currentTime + currentAnimation->getTicksPerSecond() * dt, currentAnimation->getDuration());

			float transitionTime = currentAnimation->getTicksPerSecond() * 0.2f;
			if (interpolating && interTime <= transitionTime) {
				interTime += currentAnimation->getTicksPerSecond() * dt;
				calculateBoneTransition(currentAnimation->getRootNode(), glm::mat4(1.0f), currentAnimation, nextAnimation, haltTime, interTime, transitionTime);
				return;
			}
			else if (interpolating) {
				if (queueAnimation) {
					currentAnimation = nextAnimation;
					haltTime = 0.0f;
					nextAnimation = queueAnimation;
					queueAnimation = nullptr;
					currentTime = 0.0f;
					interTime = 0.0;
					return;
				}

				interpolating = false;
				currentAnimation = nextAnimation;
				currentTime = 0.0;
				interTime = 0.0;
			}

			calculateBoneTransform(currentAnimation->getRootNode(), glm::mat4(1.0f), currentAnimation, currentTime);
		}
	}

	void playAnimation(Animation* pAnimation, bool repeat = true)
	{
		if (!currentAnimation) {
			currentAnimation = pAnimation;
			return;
		}

		if (interpolating) {
			// Handle interpolating from current interpolation here
			if (pAnimation != nextAnimation)
				queueAnimation = pAnimation;
		}
		else {
			// Else: Just playing current animation
			// Start interpolation
			if (pAnimation != nextAnimation) {
				interpolating = true;
				haltTime = fmod(currentTime, currentAnimation->getDuration());
				nextAnimation = pAnimation;
				currentTime = 0.0f;
				interTime = 0.0;
			}
		}
	}

	void calculateBoneTransition(const AssimpNodeData* curNode, glm::mat4 parentTransform, Animation* prevAnimation, Animation* nextAnimation, float haltTime, float currentTime, float transitionTime)
	{
		std::string nodeName = curNode->name;
		glm::mat4 transform = curNode->transformation;

		Bone* prevBone = prevAnimation->findBone(nodeName);
		Bone* nextBone = nextAnimation->findBone(nodeName);

		if (prevBone && nextBone)
		{
			KeyPosition prevPos = prevBone->getPositions(haltTime);
			KeyRotation prevRot = prevBone->getRotations(haltTime);
			KeyScale prevScl = prevBone->getScalings(haltTime);

			KeyPosition nextPos = nextBone->getPositions(0.0f);
			KeyRotation nextRot = nextBone->getRotations(0.0f);
			KeyScale nextScl = nextBone->getScalings(0.0f);

			prevPos.timeStamp = 0.0f;
			prevRot.timeStamp = 0.0f;
			prevScl.timeStamp = 0.0f;

			nextPos.timeStamp = transitionTime;
			nextRot.timeStamp = transitionTime;
			nextScl.timeStamp = transitionTime;

			glm::mat4 p = interpolatePosition(currentTime, prevPos, nextPos);
			glm::mat4 r = interpolateRotation(currentTime, prevRot, nextRot);
			glm::mat4 s = interpolateScaling(currentTime, prevScl, nextScl);

			transform = p * r * s;
		}

		glm::mat4 globalTransformation = parentTransform * transform;

		auto boneProps = nextAnimation->getBoneProps();
		for (unsigned int i = 0; i < boneProps.size(); i++) {
			if (boneProps[i].name == nodeName) {
				glm::mat4 offset = boneProps[i].offset;
				finalBoneMatrices[i] = globalTransformation * offset;
				break;
			}
		}

		for (int i = 0; i < curNode->childrenCount; i++)
			calculateBoneTransition(&curNode->children[i], globalTransformation, prevAnimation, nextAnimation, haltTime, currentTime, transitionTime);
	}

	void calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform, Animation* animation, float currentTime)
	{
		std::string nodeName = node->name;
		glm::mat4 boneTransform = node->transformation;

		Bone* bone = animation->findBone(nodeName);

		if (bone)
		{
			bone->update(currentTime);
			boneTransform = bone->getTransform();
		}

		glm::mat4 globalTransformation = parentTransform * boneTransform;

		auto boneProps = animation->getBoneProps();

		for (unsigned int i = 0; i < boneProps.size(); i++) {
			if (boneProps[i].name == nodeName) {
				glm::mat4 offset = boneProps[i].offset;
				finalBoneMatrices[i] = globalTransformation * offset;
				break;
			}
		}

		for (int i = 0; i < node->childrenCount; i++)
			calculateBoneTransform(&node->children[i], globalTransformation, animation, currentTime);
	}

	std::vector<glm::mat4> getFinalBoneMatrices()
	{
		return finalBoneMatrices;
	}
};

#endif