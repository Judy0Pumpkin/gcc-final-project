#version 430

layout(location = 0) in vec3 position;

uniform mat4 lightSpaceMatrix;
uniform mat4 ModelMatrix;

void main() {
    gl_Position = lightSpaceMatrix * ModelMatrix * vec4(position, 1.0);
}