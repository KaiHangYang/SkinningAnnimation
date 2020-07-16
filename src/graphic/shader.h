#pragma once

#include <glm/glm.hpp>
#include <string>
#include <GL/gl3w.h>

class Shader {
public:
	Shader() = default;
	~Shader();
	bool InitFromFile(const std::string& vs_path, const std::string& fs_path);
	bool InitFromString(const std::string& vs_str, const std::string& fs_str);

	Shader(const Shader& rhs) = delete;
	Shader& operator=(const Shader& rhs) = delete;

	void Use();
	
	void Set(const std::string& val_name, const glm::mat3& val);
	void Set(const std::string& val_name, const glm::vec3& val);
	void Set(const std::string& val_name, const glm::mat4& val);
	void Set(const std::string& val_name, const glm::vec4& val);
	void Set(const std::string& val_name, float val);
	void Set(const std::string& val_name, int val);
private:

	bool Compile(const std::string& vs_str, const std::string& fs_str);

	GLuint program_id_;
	bool inited_ = false;
};