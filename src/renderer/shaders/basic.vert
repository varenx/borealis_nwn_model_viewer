#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aColor;

out vec3 vNormal;
out vec2 vTexCoord;
out vec4 vColor;
out vec3 vFragPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position = uProjection * uView * worldPos;

    vFragPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
}
