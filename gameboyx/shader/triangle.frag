#version 450 core

layout(location = 0) in vec3 vertex_color;
//layout (location = 1) in vec2 uv;

layout(location = 0) out vec4 out_color;

void main(){
    out_color = vec4(vertex_color, 1.0);
}