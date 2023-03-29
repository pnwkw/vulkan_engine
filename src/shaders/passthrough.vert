#version 450 core

layout(location = 0) in vec3 inVert;

void main(void) {
    gl_Position = vec4(inVert, 1.0f);
}
