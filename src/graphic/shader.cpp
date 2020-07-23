#include "graphic/shader.h"
#include "common/logging.h"
#include <cstdio>
#include <filesystem/path.h>
#include <fstream>
#include <sstream>

Shader::~Shader() {}

bool Shader::CompileShader(const std::string &shader_str, GLenum shader_type,
                   GLuint &shader_id) {
  const char *shader_str_arr[] = {shader_str.c_str()};
  int shader_len_arr[] = {shader_str.size()};
  shader_id = glCreateShader(shader_type);
  glShaderSource(shader_id, 1, shader_str_arr, shader_len_arr);
  glCompileShader(shader_id);

  GLint compile_result = GL_TRUE;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compile_result);
  if (compile_result == GL_FALSE) {
    char compile_log[1024] = {'\0'};
    GLsizei log_len = 0;
    glGetShaderInfoLog(shader_id, 1024, &log_len, compile_log);
    glDeleteShader(shader_id);
    shader_id = 0;
    LOG(WARNING) << "Shader Compile Error: " << compile_log;
    return false;
  }
  return true;
}

bool Shader::LinkShader(const std::vector<GLuint> &shader_id_arr) {
  program_id_ = glCreateProgram();
  for (GLuint cur_shader : shader_id_arr) {
    glAttachShader(program_id_, cur_shader);
  }
  glLinkProgram(program_id_);
  GLint link_result;
  glGetProgramiv(program_id_, GL_LINK_STATUS, &link_result);
  if (link_result == GL_FALSE) {
    char link_log[1024] = {'\0'};
    GLsizei log_len = 0;
    glGetProgramInfoLog(program_id_, 1024, &log_len, link_log);
    glDeleteProgram(program_id_);
    program_id_ = 0;
    LOG(WARNING) << "ShaderProgram Link Error: " << link_log;
    return false;
  }
  return true;
}

bool Shader::InitFromString(const std::string &vs_str,
                            const std::string &fs_str) {
  inited_ = true;
  std::vector<GLuint> shader_id_arr(2, 0);
  inited_ =
      inited_ && CompileShader(vs_str, GL_VERTEX_SHADER, shader_id_arr[0]);
  inited_ =
      inited_ && CompileShader(fs_str, GL_FRAGMENT_SHADER, shader_id_arr[1]);
  inited_ = inited_ && LinkShader(shader_id_arr);
  return inited_;
}

bool Shader::InitFromString(const std::string &vs_str,
                            const std::string &geo_str,
                            const std::string &fs_str) {
  inited_ = true;
  std::vector<GLuint> shader_id_arr(3, 0);
  inited_ =
      inited_ && CompileShader(vs_str, GL_VERTEX_SHADER, shader_id_arr[0]);
  inited_ =
      inited_ && CompileShader(geo_str, GL_GEOMETRY_SHADER, shader_id_arr[1]);
  inited_ =
      inited_ && CompileShader(fs_str, GL_FRAGMENT_SHADER, shader_id_arr[2]);
  inited_ = inited_ && LinkShader(shader_id_arr);
  return inited_;
}

bool Shader::InitFromFile(const std::string &vs_path,
                          const std::string &fs_path) {
  inited_ = true;
  if (!filesystem::path(vs_path).is_file()) {
    LOG(WARNING) << "Shader Error: " << vs_path << "is not a file!";
    inited_ = inited_ && false;
    return inited_;
  }

  if (!filesystem::path(fs_path).is_file()) {
    LOG(WARNING) << "Shader Error: " << fs_path << "is not a file!";
    inited_ = inited_ && false;
    return inited_;
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

  inited_ = inited_ && InitFromString(vs_str, fs_str);
  return inited_;
}

bool Shader::InitFromFile(const std::string &vs_path,
                          const std::string &geo_path,
                          const std::string &fs_path) {
  inited_ = true;
  if (!filesystem::path(vs_path).is_file()) {
    LOG(WARNING) << "Shader Error: " << vs_path << "is not a file!";
    inited_ = inited_ && false;
    return inited_;
  }

  if (!filesystem::path(geo_path).is_file()) {
    LOG(WARNING) << "Shader Error: " << geo_path << "is not a file!";
    inited_ = inited_ && false;
    return inited_;
  }

  if (!filesystem::path(fs_path).is_file()) {
    LOG(WARNING) << "Shader Error: " << fs_path << "is not a file!";
    inited_ = inited_ && false;
    return inited_;
  }

  // read vertex shader
  std::string vs_str;
  std::ifstream vs_file(vs_path);
  std::stringstream vs_stream;
  vs_stream << vs_file.rdbuf();
  vs_str = vs_stream.str();

  // read geometry shader
  std::string geo_str;
  std::ifstream geo_file(geo_path);
  std::stringstream geo_stream;
  geo_stream << geo_file.rdbuf();
  geo_str = geo_stream.str();

  // read fragment shader
  std::string fs_str;
  std::ifstream fs_file(fs_path);
  std::stringstream fs_stream;
  fs_stream << fs_file.rdbuf();
  fs_str = fs_stream.str();

  inited_ = inited_ && InitFromString(vs_str, geo_str, fs_str);
  return inited_;
}

void Shader::Use() {
  if (inited_)
    glUseProgram(program_id_);
  else
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
}

void Shader::SetMat4Array(const std::string &val_name, const std::vector<float> &val, int n) {
  if (inited_) {
    CHECK(val.size() == n * 16) << "val size doesn't match 16 * n";
    GLint location = glGetUniformLocation(program_id_, val_name.c_str());
    glUniformMatrix4fv(location, n, GL_FALSE, val.data());
  }
  else {
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
  }
}

void Shader::Set(const std::string &val_name, const glm::mat3 &val) {
  if (inited_) {
    GLint location = glGetUniformLocation(program_id_, val_name.c_str());
    glUniformMatrix3fv(location, 1, GL_FALSE, &val[0][0]);
  } else {
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
  }
}

void Shader::Set(const std::string &val_name, const glm::vec3 &val) {
  if (inited_) {
    GLint location = glGetUniformLocation(program_id_, val_name.c_str());
    glUniform3f(location, val.x, val.y, val.z);
  } else {
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
  }
}

void Shader::Set(const std::string &val_name, const glm::mat4 &val) {
  if (inited_) {
    GLint location = glGetUniformLocation(program_id_, val_name.c_str());
    glUniformMatrix4fv(location, 1, GL_FALSE, &val[0][0]);
  } else {
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
  }
}

void Shader::Set(const std::string &val_name, const glm::vec4 &val) {
  if (inited_) {
    GLint location = glGetUniformLocation(program_id_, val_name.c_str());
    glUniform4f(location, val.x, val.y, val.z, val.w);
  } else {
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
  }
}

void Shader::Set(const std::string &val_name, float val) {
  if (inited_) {
    GLint location = glGetUniformLocation(program_id_, val_name.c_str());
    glUniform1f(location, val);
  } else {
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
  }
}

void Shader::Set(const std::string &val_name, int val) {
  if (inited_) {
    GLint location = glGetUniformLocation(program_id_, val_name.c_str());
    glUniform1i(location, val);
  } else {
    LOG(WARNING) << "Shader Error: Shader hasn't been inited!";
  }
}