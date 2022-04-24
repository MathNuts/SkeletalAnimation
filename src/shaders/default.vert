#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangents;
layout (location = 4) in vec3 aBitangents;
layout (location = 5) in ivec4 boneIds; 
layout (location = 6) in vec4 weights;

layout (location = 0) uniform mat4 M;
layout (location = 1) uniform mat4 V;
layout (location = 2) uniform mat4 P;
layout (location = 4) uniform uint type;

out vec3 normal;
out vec3 FragPos;
out vec2 texCoords;
out vec3 tangents;
out vec3 bitangents;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 boneTransforms[MAX_BONES];

void main()
{
    vec4 updatedPosition = vec4(0.0f);
    vec3 updatedNormal = vec3(0.0f);

    if(type == 5) {
        for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
        {
            // Current bone-weight pair is non-existing
            if(boneIds[i] == -1) 
                continue;

            // Ignore all bones over count MAX_BONES
            if(boneIds[i] >= MAX_BONES) 
            {
                updatedPosition = vec4(aPos,1.0f);
                break;
            }
            // Set pos
            vec4 localPosition = boneTransforms[boneIds[i]] * vec4(aPos,1.0f);
            updatedPosition += localPosition * weights[i];
            // Set normal
            vec3 localNormal = mat3(boneTransforms[boneIds[i]]) * aNormal;
            updatedNormal += localNormal * weights[i];
        }
    } else {
        updatedPosition = vec4(aPos, 1.0f);
        updatedNormal = aNormal;
    }
 
    gl_Position = P * V * M * updatedPosition;
    FragPos = vec3(M * vec4(vec3(updatedPosition), 1.0));
    normal = updatedNormal;
    texCoords = aTexCoords;
    tangents = aTangents;
    bitangents = aBitangents;
}