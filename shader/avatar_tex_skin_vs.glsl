#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord0;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_skinning_joints;
layout(location = 4) in vec4 in_skinning_weights;

out vec3 frag_normal;
out vec3 frag_position;
out vec4 frag_color;
out vec2 frag_texcoord;

uniform mat4 model_matrix;
uniform mat4 view_matrix;
uniform mat4 proj_matrix;
uniform vec4 vertex_color;

uniform mat4 skinning_transforms[100];

void main() {
  // lbs
  mat4 skinning_matrix =
      in_skinning_weights.x * skinning_transforms[int(in_skinning_joints.x)] +
      in_skinning_weights.y * skinning_transforms[int(in_skinning_joints.y)] +
      in_skinning_weights.z * skinning_transforms[int(in_skinning_joints.z)] +
      in_skinning_weights.w * skinning_transforms[int(in_skinning_joints.w)];

  vec4 skin_position = skinning_matrix * vec4(in_position, 1.0f);
  vec3 skin_normal = mat3(skinning_matrix) * in_normal;

  gl_Position = proj_matrix * view_matrix * model_matrix * skin_position;

  frag_position = vec3(view_matrix * model_matrix * skin_position);
  frag_normal = mat3(transpose(inverse(model_matrix))) * skin_normal;
  frag_color = vertex_color;
  frag_texcoord = in_texcoord0;
}