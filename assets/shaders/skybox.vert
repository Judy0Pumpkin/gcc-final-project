#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 TexCoords;
uniform mat4 Projection;
uniform mat4 ViewMatrix;

// TODO#3-1: vertex shader

void main() {
    TexCoords = aPos;
    mat4 view = ViewMatrix;
    view[3][0] = 0.0;
    view[3][1] = 0.0;
    view[3][2] = 0.0;
    vec4 pos = Projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
