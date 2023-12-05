#version 450 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec3 in_color;

layout(location = 0) out vec3 vertex_color;
layout (location = 1) out vec2 uv;

void main(){
    uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    vertex_color = in_color;
    gl_Position = vec4(in_position, 0.0f, 1.0f);
}