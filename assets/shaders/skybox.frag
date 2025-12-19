#version 330 core
in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

// TODO#3-2: fragment shader

void main() {
    FragColor = texture(skybox, TexCoords);
}

