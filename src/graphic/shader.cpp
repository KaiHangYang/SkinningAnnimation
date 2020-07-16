#include "common/logging.h"
#include "graphic/shader.h"
#include <filesystem/path.h>
#include <cstdio>
#include <fstream>
#include <sstream>

Shader::~Shader() {}

bool Shader::Compile(const std::string& vs_str, const std::string& fs_str) {
	const char* vs_str_arr[] = { vs_str.c_str() };
	int vs_len_arr[] = { vs_str.size() };
	GLuint vs_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs_shader, 1, vs_str_arr, vs_len_arr);
	glCompileShader(vs_shader);

	GLint compile_result = GL_TRUE;
	glGetShaderiv(vs_shader, GL_COMPILE_STATUS, &compile_result);
	if (compile_result == GL_FALSE) {
		char compile_log[1024] = { '\0' };
		GLsizei log_len = 0;
		glGetShaderInfoLog(vs_shader, 1024, &log_len, compile_log);
		glDeleteShader(vs_shader);
		vs_shader = 0;
		LOG(WARNING) << "VertexShader Compile Error: " << compile_log;
		return false;
	}

	const char* fs_str_arr[] = { fs_str.c_str() };
	int fs_len_arr[] = { fs_str.size() };
	GLuint fs_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs_shader, 1, fs_str_arr, fs_len_arr);
	glCompileShader(fs_shader);

	compile_result = GL_TRUE;
	glGetShaderiv(fs_shader, GL_COMPILE_STATUS, &compile_result);
	if (compile_result == GL_FALSE) {
		char complie_log[1024] = { '\0' };
		GLsizei log_len = 0;
		glGetShaderInfoLog(fs_shader, 1024, &log_len, complie_log);
		glDeleteShader(fs_shader);
		fs_shader = 0;
		LOG(WARNING) << "FragmentShader Compile Error: " << complie_log;
		return false;
	}

	program_id_ = glCreateProgram();
	glAttachShader(program_id_, vs_shader);
	glAttachShader(program_id_, fs_shader);
	glLinkProgram(program_id_);
	GLint link_result;
	glGetProgramiv(program_id_, GL_LINK_STATUS, &link_result);
	if (link_result == GL_FALSE) {
		char link_log[1024] = { '\0' };
		GLsizei log_len = 0;
		glGetProgramInfoLog(program_id_, 1024, &log_len, link_log);
		glDeleteProgram(program_id_);
		program_id_ = 0;
		LOG(WARNING) << "ShaderProgram Link Error: " << link_log;
		return false;
	}
	return true;
}

bool Shader::InitFromString(const std::string& vs_str, const std::string& fs_str) {
	inited_ = Compile(vs_str, fs_str);
	return inited_;
}

bool Shader::InitFromFile(const std::string& vs_path, const std::string& fs_path) {
	inited_ = false;
	if (!filesystem::path(vs_path).is_file()) {
		LOG(WARNING) << "Shader Error: " << vs_path << "is not a file!";
		return false;
	}
	
	if (!filesystem::path(fs_path).is_file()) {
		LOG(WARNING) << "Shader Error: " << fs_path << "is not a file!";
		return false;
	}

	// read vertex shader
	std::string vs_str;
	std::ifstream vs_file(vs_path);
	std::stringstream vs_stream;
	vs_stream << vs_file.rdbuf();
	vs_str = vs_stream.str();
	
	// read fragment shader
	std::string fs_str;
	std::ifstream fs_file(fs_path);
	std::stringstream fs_stream;
	fs_stream << fs_file.rdbuf();
	fs_str = fs_stream.str();
	
	inited_ = Compile(vs_str, fs_str);

	return inited_;
}

void Shader::Use() {
	if (inited_) glUseProgram(program_id_);
	else LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
}

void Shader::Set(const std::string& val_name, const glm::mat3& val) {
	if (inited_) {
		GLint location = glGetUniformLocation(program_id_, val_name.c_str());
		glUniformMatrix3fv(location, 1, GL_FALSE, &val[0][0]);
	}
	else {
		LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
	}
}

void Shader::Set(const std::string& val_name, const glm::vec3& val) {
	if (inited_) {
		GLint location = glGetUniformLocation(program_id_, val_name.c_str());
		glUniform3f(location, val.x, val.y, val.z);
	}
	else {
		LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
	}
}

void Shader::Set(const std::string& val_name, const glm::mat4& val) {
	if (inited_) {
		GLint location = glGetUniformLocation(program_id_, val_name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, &val[0][0]);
	}
	else {
		LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
	}
}

void Shader::Set(const std::string& val_name, const glm::vec4& val) {
	if (inited_) {
		GLint location = glGetUniformLocation(program_id_, val_name.c_str());
		glUniform4f(location, val.x, val.y, val.z, val.w);
	}
	else {
		LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
	}
}

void Shader::Set(const std::string& val_name, float val) {
	if (inited_) {
		GLint location = glGetUniformLocation(program_id_, val_name.c_str());
		glUniform1f(location, val);
	}
	else {
		LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
	}
}

void Shader::Set(const std::string& val_name, int val) {
	if (inited_) {
		GLint location = glGetUniformLocation(program_id_, val_name.c_str());
		glUniform1i(location, val);
	}
	else {
		LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
	}
}