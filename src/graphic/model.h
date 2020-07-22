#pragma once

#include <GL/gl3w.h>
#include <memory>
#include <string>
#include <tiny_gltf.h>
#include <vector>
#include <set>
#include <glm/glm.hpp>

#include "graphic/shader.h"

class Model {
public:
  enum DrawType {DRAW_ARRAY = 0, DRAW_ELEMENT = 1};
  Model() = default;
  void Init(const std::string &model_path, const std::shared_ptr<Shader> &shader);
  void Render(const glm::mat4& view_matrix, const glm::mat4& proj_matrix, const glm::mat4& model_matrix);
  ~Model(){};

private:
  struct RenderParams {
    DrawType draw_type = DRAW_ARRAY;
    int count = 0;
    GLuint vao = 0;
    GLuint position_vbo = 0;
    GLuint texcoord_vbo = 0;
    GLuint normal_vbo = 0;
    GLuint indices_vbo = 0;
  };

  void RenderNode(const tinygltf::Node& node, const glm::mat4& parent_transform);
  void RenderMesh(int mesh_idx);
  GLuint ProcessBufferView(const tinygltf::Accessor& accessor, GLenum buffer_type);
  size_t GLTFComponentByteSize(int type);
  tinygltf::Model model_;
  std::shared_ptr<Shader> shader_;

  std::map<int, std::vector<RenderParams>> mesh_render_params_;
  std::map<int, GLuint> gpu_buffer_views_;
};