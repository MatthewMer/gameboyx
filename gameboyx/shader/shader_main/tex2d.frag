#version 450 core

layout (location = 0) in vec2 in_uvs;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform sampler2D in_texture2d;

void main(){
    vec4 tex_sample = texture(in_texture2d, in_uvs);
    out_color = tex_sample;
}