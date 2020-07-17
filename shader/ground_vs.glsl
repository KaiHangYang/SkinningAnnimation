#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec4 vColor;

out vec3 frag_normal;
out vec3 frag_position;
out vec4 frag_color;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;

void main() {
  gl_Position = proj_matrix * view_matrix * model_matrix * vec4(position, 1.0f);
  frag_position = vec3(view_matrix * model_matrix * vec4(position, 1.0f));
  frag_color = vColor;
  frag_normal = mat3(transpose(inverse(model_matrix))) * normal;
}