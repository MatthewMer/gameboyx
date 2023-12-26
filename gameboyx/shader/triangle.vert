#version 450 core

layout(location = 0) in vec2 in_position;

layout(push_constant) uniform pushConstants{
    mat4 model_view_proj;
} push_const;

layout(location = 0) out vec2 uv;

void main(){
    mat4 model_view_proj = push_const.model_view_proj;
    uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = model_view_proj * vec4(in_position, 0.0f, 1.0f);
}