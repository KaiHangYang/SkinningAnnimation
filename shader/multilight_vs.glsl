#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 vColor;
layout (location = 2) in vec3 normal;

out vec3 Normal;
out vec3 FragPos;
out vec3 fColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 normMat; // inverse and transposed mat
uniform vec3 fragColor;

// 0 render the model, 1 render the scene
uniform int renderType;

void main()
{
    gl_Position = projection * view *  model * vec4(position, 1.0f);

    FragPos = vec3(model * vec4(position, 1.0f));

    if (renderType == 0) {
        fColor = fragColor;
        Normal = mat3(normMat) * normal;
        //Normal = mat3(transpose(inverse(model))) * normal;  
    }
    else {
        fColor = vColor;
        Normal = mat3(normMat) * vec3(0.0, 1.0, 0.0);
    }
} 
