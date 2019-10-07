#version 330 core

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_tex_coord;

out vec2 tex_coord;

void main(void) {
    tex_coord = in_tex_coord;
    gl_Position = vec4(in_position, -0.99999, 1.0);
}