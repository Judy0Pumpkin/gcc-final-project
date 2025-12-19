#version 430

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

uniform mat4 Projection;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;
uniform mat4 TIModelMatrix;
uniform mat4 lightSpaceMatrix;
uniform mat4 spotLightSpaceMatrix;

out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec4 FragPosLightSpace;
out vec4 FragPosSpotLightSpace;   

// TODO#3-3: vertex shader


void main() {
    FragPos = vec3(ModelMatrix * vec4(position, 1.0));
    Normal = mat3(TIModelMatrix) * normal;
    TexCoord = texCoord;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    FragPosSpotLightSpace = spotLightSpaceMatrix * vec4(FragPos, 1.0);  // ✅ 新增
    gl_Position = Projection * ViewMatrix * ModelMatrix * vec4(position, 1.0);
}