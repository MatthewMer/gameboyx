#version 450 core

vec3 colors[3] = vec3[](
    vec3(1.0,0.0,0.0),
    vec3(0.0,1.0,0.0),
    vec3(0.0,0.0,1.0)
);

layout(location = 0) out vec3 vertex_color;
layout (location = 1) out vec2 uv;

void main(){
    uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(uv * 2.0f + -1.0f, 0.0f, 1.0f);
    vertex_color = colors[gl_VertexIndex];
}