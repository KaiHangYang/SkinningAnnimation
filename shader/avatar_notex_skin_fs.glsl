#version 330 core

in vec3 frag_normal;
in vec3 frag_position;
in vec4 frag_color;

out vec4 color;

void main() {
  color = frag_color;
}