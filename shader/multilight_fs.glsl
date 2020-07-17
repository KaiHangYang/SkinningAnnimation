#version 330 core

#define NR_POINT_LIGHTS 2

struct PointLight {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};


in vec3 FragPos;
in vec3 Normal;
in vec3 fColor;

out vec4 color;

uniform vec3 viewPos;
uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform samplerCube depth_cube[NR_POINT_LIGHTS];

uniform bool use_shading;
uniform float far_plane;
uniform int use_shadow;
uniform float shadow_bias;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, int light_index);
float shadowCalculation(vec3 fragPos, int light_index);

void main() {    
    if (use_shading) {
        // Properties

    //    if (!gl_FrontFacing) {
    //        color = vec4(0.3, 0.3, 0.3, 1.0);
    //        return;
    //    }
        vec3 norm = normalize(Normal);
        vec3 viewDir = normalize(viewPos - FragPos);

        vec3 result = vec3(0.0f, 0.0f, 0.0f);

        for(int i = 0; i < NR_POINT_LIGHTS; i++) {
            result += 1.f / NR_POINT_LIGHTS * CalcPointLight(pointLights[i], norm, FragPos, viewDir, i);
        }
        color = vec4(result, 1.0);
    }
    else {
        color = vec4(fColor, 1.0);
    }
}

float shadowCalculation(vec3 fragPos, int light_index) {
    // get vector between fragment position and light position
    float shadow = 0.0;
    float bias = shadow_bias; // we use a much larger bias since depth is now in [near_plane, far_plane] range

    float samples = 4.0;
    float offset = 0.5;
    vec3 fragToLight = fragPos - pointLights[light_index].position;
    float currentDepth = length(fragToLight);
    float closestDepth;

    for (float x = -offset; x < offset; x += offset / (samples * 0.5)) {
        for (float y = -offset; y < offset; y += offset / (samples * 0.5)) {
            for (float z = -offset; z < offset; z += offset / (samples * 0.5)) {
                closestDepth = texture(depth_cube[light_index], fragToLight).r;
                closestDepth *= far_plane;
                if (currentDepth - bias > closestDepth) {
                    shadow += 1.0;
                }
            }
        }
    }

    shadow /= (samples * samples * samples);
    return shadow;
}
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, int light_index) {
    vec3 lightDir = normalize(light.position - fragPos);
    // Diffuse shading
    float diff = max(dot(lightDir, normal), 0.0);
    // Specular shading
    vec3 reflectDir = normalize(reflect(-lightDir, normal));

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);

    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    vec3 specular = light.specular * spec;

    float shadow = 0.0;
    if (use_shadow == 1) {
        shadow = shadowCalculation(fragPos, light_index);
    }

    return (ambient + (1.0 - shadow) * (diffuse + specular)) * fColor;
}
