#pragma once

#include <glm/glm.hpp>
#include <string>
#include <GL/gl3w.h>

class Shader {
public:
	Shader(const std::string& vs_path, const std::string& fs_path);
	~Shader();

	void Use();
	
	void Set(const std::string& val_name, const glm::mat3& val);
	void Set(const std::string& val_name, const glm::vec3& val);
	void Set(const std::string& val_name, const glm::mat4& val);
	void Set(const std::string& val_name, const glm::vec4& val);
	void Set(const std::string& val_name, float val);
private:
	GLuint program_id_;
};