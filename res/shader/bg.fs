#version 330 core

in vec2 tex_coord;
uniform sampler2D tex;

out vec4 color;

void main(void) {
    color = texture(tex, tex_coord);
}