#include "common/logging.h"
#include "common/shader.h"
#include <filesystem/path.h>
#include <cstdio>
#include <fstream>
#include <sstream>

Shader::~Shader() {}

Shader::Shader(const std::string& vs_path, const std::string& fs_path) {
	CHECK(filesystem::path(vs_path).is_file()) <<
		"Shader Error: " << vs_path << "is not a file!";
	CHECK(filesystem::path(fs_path).is_file()) <<
		"Shader Error: " << fs_path << "is not a file!";

	// load vertex shader
	std::string vs_str;
	std::ifstream vs_file(vs_path);
	std::stringstream vs_stream;
	vs_stream << vs_file.rdbuf();
	vs_str = vs_stream.str();
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
		LOG(FATAL) << "VertexShader Compile Error: " << compile_log;
	}

	// read fragment shader
	std::string fs_str;
	std::ifstream fs_file(fs_path);
	std::stringstream fs_stream;
	fs_stream << fs_file.rdbuf();
	fs_str = fs_stream.str();
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
		LOG(FATAL) << "FragmentShader Compile Error: " << complie_log;
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
		LOG(FATAL) << "ShaderProgram Link Error: " << link_log;
	}
}

void Shader::Use() {
	glUseProgram(this->program_id_);
}

void Shader::Set(const std::string& val_name, const glm::mat3& val) {
	GLint localtion = glGetUniformLocation(program_id_, val_name.c_str());
	glUniformMatrix3fv(localtion, 1, GL_FALSE, &val[0][0]);
}

void Shader::Set(const std::string& val_name, const glm::vec3& val) {
	GLint localtion = glGetUniformLocation(program_id_, val_name.c_str());
	glUniform3f(localtion, val.x, val.y, val.z);
}

void Shader::Set(const std::string& val_name, const glm::mat4& val) {
	GLint localtion = glGetUniformLocation(program_id_, val_name.c_str());
	glUniformMatrix4fv(localtion, 1, GL_FALSE, &val[0][0]);
}

void Shader::Set(const std::string& val_name, const glm::vec4& val) {
	GLint localtion = glGetUniformLocation(program_id_, val_name.c_str());
	glUniform4f(localtion, val.x, val.y, val.z, val.w);
}
void Shader::Set(const std::string& val_name, float val) {
	GLint localtion = glGetUniformLocation(program_id_, val_name.c_str());
	glUniform1f(localtion, val);
}