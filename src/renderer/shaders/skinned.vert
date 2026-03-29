#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;
layout(location = 4) in vec4 aBoneWeights;
layout(location = 5) in ivec4 aBoneIndices;

out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;
out vec3 vFragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uBoneMatrices[17];  // Up to 17 bones per skinmesh

void main() {
    // Skinning transformation
    mat4 skinMatrix =
        aBoneWeights.x * uBoneMatrices[aBoneIndices.x] +
        aBoneWeights.y * uBoneMatrices[aBoneIndices.y] +
        aBoneWeights.z * uBoneMatrices[aBoneIndices.z] +
        aBoneWeights.w * uBoneMatrices[aBoneIndices.w];

    vec4 skinnedPos = skinMatrix * vec4(aPosition, 1.0);
    vec4 worldPos = uModel * skinnedPos;

    gl_Position = uProjection * uView * worldPos;

    vFragPos = worldPos.xyz;
    vNormal = mat3(skinMatrix) * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
}
