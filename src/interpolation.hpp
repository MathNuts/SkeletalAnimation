#ifndef INTERPOLATION_HPP
#define INTERPOLATION_HPP

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

struct KeyPosition
{
	glm::vec3 position;
	float timeStamp;
};

struct KeyRotation
{
	glm::quat orientation;
	float timeStamp;
};

struct KeyScale
{
	glm::vec3 scale;
	float timeStamp;
};


float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime)
{
	float scaleFactor = 0.0f;
	float midWayLength = animationTime - lastTimeStamp;
	float framesDiff = nextTimeStamp - lastTimeStamp;
	scaleFactor = midWayLength / framesDiff;
	return scaleFactor;
}

glm::mat4 interpolatePosition(float animationTime, KeyPosition from, KeyPosition to)
{
	float scaleFactor = getScaleFactor(from.timeStamp, to.timeStamp, animationTime);
	glm::vec3 finalPosition = glm::mix(from.position, to.position, scaleFactor);
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), finalPosition);
	return translation;
}

glm::mat4 interpolateRotation(float animationTime, KeyRotation from, KeyRotation to)
{
	float scaleFactor = getScaleFactor(from.timeStamp, to.timeStamp, animationTime);
	glm::quat finalRotation = glm::slerp(from.orientation, to.orientation, scaleFactor);
	finalRotation = glm::normalize(finalRotation);
	return glm::toMat4(finalRotation);
}

glm::mat4 interpolateScaling(float animationTime, KeyScale from, KeyScale to)
{
	float scaleFactor = getScaleFactor(from.timeStamp, to.timeStamp, animationTime);
	glm::vec3 finalScale = glm::mix(from.scale, to.scale, scaleFactor);
	return glm::scale(glm::mat4(1.0f), finalScale);
}

#endif