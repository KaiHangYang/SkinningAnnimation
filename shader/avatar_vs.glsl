#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 frag_normal;
out vec3 frag_position;
out vec4 frag_color;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;

void main() {
  gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position, 1.0f);
  frag_position = vec3(view_matrix * model_matrix * vec4(position, 1.0f));
  frag_color = vec4(0.56, 0.2, 0.8, 1.0);
  frag_normal = mat3(transpose(inverse(model_matrix))) * normal;
}