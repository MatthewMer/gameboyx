#version 450 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uvs;

layout(push_constant) uniform pushConstants{
    mat4 model_view_proj;
} push_const;

layout(location = 0) out vec2 out_uvs;

void main(){
    out_uvs = in_uvs;
    mat4 model_view_proj = push_const.model_view_proj;
    gl_Position = model_view_proj * vec4(in_position, 0.0f, 1.0f);
}