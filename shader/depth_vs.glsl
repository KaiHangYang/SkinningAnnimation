#version 330 core

layout (location = 0) in vec3 v_position;

uniform mat4 model;

void main() {
	gl_Position = model * vec4(v_position, 1.0f);
}
