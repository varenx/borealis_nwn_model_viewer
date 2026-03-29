#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
in vec4 vColor;
in vec3 vFragPos;

out vec4 fragColor;

uniform sampler2D uTexture;
uniform bool uHasTexture;
uniform vec3 uDiffuse;
uniform vec3 uAmbient;
uniform vec3 uSpecular;
uniform float uShininess;
uniform float uAlpha;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uViewPos;

void main() {
    vec3 normal = normalize(vNormal);

    // Flip normal if facing away (for two-sided lighting)
    if (!gl_FrontFacing) {
        normal = -normal;
    }

    // Ambient
    vec3 ambient = uAmbient * uLightColor * 0.3;

    // Diffuse
    float diff = max(dot(normal, -uLightDir), 0.0);
    vec3 diffuse = diff * uDiffuse * uLightColor;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 halfDir = normalize(-uLightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), max(uShininess, 1.0));
    vec3 specular = spec * uSpecular * uLightColor;

    vec3 result = ambient + diffuse + specular;

    vec4 baseColor = vColor;
    if (uHasTexture) {
        baseColor *= texture(uTexture, vTexCoord);
    }

    fragColor = vec4(result * baseColor.rgb, baseColor.a * uAlpha);
}
