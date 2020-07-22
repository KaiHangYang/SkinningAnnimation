#version 330 core

in vec3 frag_normal;
in vec3 frag_position;
in vec4 frag_color;
in vec2 frag_texcoord;

uniform int has_texture;
uniform sampler2D diffuse_texture;

out vec4 color;

void main() {
  if (has_texture == 1) {
    color = texture2D(diffuse_texture, frag_texcoord);
  }
  else {
    color = frag_color;
  }
}