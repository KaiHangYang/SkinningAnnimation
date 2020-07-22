#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "common/logging.h"
#include "graphic/model.h"

void Model::Init(const std::string &model_path,
                 const std::shared_ptr<Shader> &shader) {
  shader_ = shader;

  // Load models.
  tinygltf::TinyGLTF gltf_ctx;
  std::string err;
  std::string warn;
  bool ret = false;
  ret = gltf_ctx.LoadASCIIFromFile(&model_, &err, &warn, model_path);

  if (!err.empty()) {
    LOG(ERROR) << err;
  }
  if (!warn.empty()) {
    LOG(WARNING) << warn;
  }
  if (!ret) {
    LOG(FATAL) << "Load gltf failed!";
  }

  mesh_render_params_.clear();

  for (size_t m_idx = 0; m_idx < model_.meshes.size(); ++m_idx) {
    const auto &mesh = model_.meshes[m_idx];
    mesh_render_params_[m_idx] = std::vector<RenderParams>();
    for (size_t p_idx = 0; p_idx < mesh.primitives.size(); ++p_idx) {
      const auto &primitive = mesh.primitives[p_idx];
      RenderParams cur_render_params;
      glGenVertexArrays(1, &cur_render_params.vao);
      glBindVertexArray(cur_render_params.vao);

      auto a_iter = primitive.attributes.begin();
      auto a_iter_end = primitive.attributes.end();
      for (; a_iter != a_iter_end; ++a_iter) {
        const auto &accessor = model_.accessors[a_iter->second];
        GLuint cur_vbo = 0;
        if (a_iter->first == "POSITION" || a_iter->first == "NORMAL" ||
            a_iter->first == "TEXCOORD_0" || a_iter->first == "JOINTS_0" ||
            a_iter->first == "WEIGHTS_0") {
          if (gpu_buffer_views_.find(accessor.bufferView) ==
              gpu_buffer_views_.end()) {
            cur_vbo = ProcessBufferView(accessor, GL_ARRAY_BUFFER);
          } else {
            cur_vbo = gpu_buffer_views_[accessor.bufferView];
          }
        }
        int size = 1;
        if (accessor.type == TINYGLTF_TYPE_SCALAR) {
          size = 1;
        } else if (accessor.type == TINYGLTF_TYPE_VEC2) {
          size = 2;
        } else if (accessor.type == TINYGLTF_TYPE_VEC3) {
          size = 3;
        } else if (accessor.type == TINYGLTF_TYPE_VEC4) {
          size = 4;
        } else {
          CHECK(0) << "Don't support accessor.type: " << accessor.type;
        }
        int byte_stride =
            accessor.ByteStride(model_.bufferViews[accessor.bufferView]);
        CHECK(byte_stride != -1) << "byte_strid equal -1";
        if (a_iter->first == "POSITION") {
          cur_render_params.position_vbo = cur_vbo;
          cur_render_params.count = accessor.count;
          glBindBuffer(GL_ARRAY_BUFFER, cur_render_params.position_vbo);
          glVertexAttribPointer(0, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(0);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (a_iter->first == "TEXCOORD_0") {
          cur_render_params.texcoord_vbo = cur_vbo;
          glBindBuffer(GL_ARRAY_BUFFER, cur_render_params.texcoord_vbo);
          glVertexAttribPointer(1, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(1);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (a_iter->first == "NORMAL") {
          cur_render_params.normal_vbo = cur_vbo;
          glBindBuffer(GL_ARRAY_BUFFER, cur_render_params.normal_vbo);
          glVertexAttribPointer(2, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(2);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (a_iter->first == "JOINTS_0") {
        } else if (a_iter->first == "WEIGHTS_0") {
        }
      }

      if (primitive.indices >= 0) {
        cur_render_params.draw_type = DRAW_ELEMENT;
        cur_render_params.indices_vbo = ProcessBufferView(
            model_.accessors[primitive.indices], GL_ELEMENT_ARRAY_BUFFER);
        cur_render_params.count = model_.accessors[primitive.indices].count;
      } else {
        cur_render_params.draw_type = DRAW_ARRAY;
      }

      // Load textures.
      if (primitive.material >= 0) {
        const auto &material = model_.materials[primitive.material];
        if (material.pbrMetallicRoughness.baseColorTexture.index >= 0) {
          // Load image as texture.
          const auto &texture = model_.textures[material.pbrMetallicRoughness
                                                    .baseColorTexture.index];
          const auto &sampler = model_.samplers[texture.sampler];
          const auto &image = model_.images[texture.source];

          glGenTextures(1, &cur_render_params.texture_id);glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
          glBindTexture(GL_TEXTURE_2D, cur_render_params.texture_id);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                          sampler.minFilter);
          glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                          sampler.magFilter);
          GLenum format = GL_RGBA;
          if (image.component == 3) {
            format = GL_RGB;
          }

          GLenum type = GL_UNSIGNED_BYTE;
          if (image.bits == 8) {
            type = GL_UNSIGNED_BYTE;
          } else if (image.bits == 16) {
            type = GL_UNSIGNED_SHORT;
          } else if (image.bits == 32) {
            type = GL_UNSIGNED_INT;
          } else {
            CHECK(0) << "Don't support image.bits: " << image.bits;
          }
          glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0,
                       format, type, image.image.data());
          glGenerateMipmap(GL_TEXTURE_2D);
          glBindTexture(GL_TEXTURE_2D, 0);
          // model_.samplers
        } else {
          if (!material.pbrMetallicRoughness.baseColorFactor.empty()) {
            cur_render_params.color =
                glm::vec4(material.pbrMetallicRoughness.baseColorFactor[0],
                          material.pbrMetallicRoughness.baseColorFactor[1],
                          material.pbrMetallicRoughness.baseColorFactor[2],
                          material.pbrMetallicRoughness.baseColorFactor[3]);
          }
          else {
            cur_render_params.color = glm::vec4(0.5, 0.5, 0.5, 1.0);
          }
        }
      }

      mesh_render_params_[m_idx].push_back(cur_render_params);
      glBindVertexArray(0);
    }
  }
}

void Model::Render(const glm::mat4 &view_matrix, const glm::mat4 &proj_matrix,
                   const glm::mat4 &model_matrix) {
  shader_->Use();
  shader_->Set("model_matrix", model_matrix);
  shader_->Set("view_matrix", view_matrix);
  shader_->Set("proj_matrix", proj_matrix);
  GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
  GLboolean last_enable_multisample = glIsEnabled(GL_MULTISAMPLE);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  int scene_to_display = model_.defaultScene > -1 ? model_.defaultScene : 0;
  const tinygltf::Scene &scene = model_.scenes[scene_to_display];
  for (size_t n_idx = 0; n_idx < scene.nodes.size(); ++n_idx) {
    RenderNode(model_.nodes[scene.nodes[n_idx]], glm::mat4(1.f));
  }

  if (!last_enable_depth_test)
    glDisable(GL_DEPTH_TEST);
  if (!last_enable_multisample)
    glDisable(GL_MULTISAMPLE);
}

size_t Model::GLTFComponentByteSize(int type) {
  switch (type) {
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
  case TINYGLTF_COMPONENT_TYPE_BYTE:
    return sizeof(char);
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
  case TINYGLTF_COMPONENT_TYPE_SHORT:
    return sizeof(short);
  case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
  case TINYGLTF_COMPONENT_TYPE_INT:
    return sizeof(int);
  case TINYGLTF_COMPONENT_TYPE_FLOAT:
    return sizeof(float);
  case TINYGLTF_COMPONENT_TYPE_DOUBLE:
    return sizeof(double);
  default:
    return 0;
  }
};

void Model::RenderNode(const tinygltf::Node &node,
                       const glm::mat4 &parent_transform) {
  glm::mat4 cur_transform(1.f);
  if (node.matrix.size() == 16) {
    std::copy(node.matrix.begin(), node.matrix.end(), &cur_transform[0][0]);
    cur_transform = cur_transform;
  } else {
    glm::vec3 scale_vec(1.0, 1.0, 1.0);
    glm::quat rot_vec(1.0, 0.0, 0.0, 0.0);
    glm::vec3 trans_vec(0.0, 0.0, 0.0);
    if (!node.scale.empty()) {
      scale_vec = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
    }
    if (!node.rotation.empty()) {
      rot_vec = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1],
                          node.rotation[2]);
    }
    if (!node.translation.empty()) {
      trans_vec = glm::vec3(node.translation[0], node.translation[1],
                            node.translation[2]);
    }
    glm::mat4 cur_scale = glm::scale(glm::mat4(1.f), scale_vec);
    glm::mat4 cur_rot = glm::toMat4(rot_vec);
    glm::mat4 cur_trans = glm::translate(glm::mat4(1.f), trans_vec);
    cur_transform = cur_trans * cur_rot * cur_scale;
  }
  cur_transform = parent_transform * cur_transform;

  if (node.mesh > -1) {
    RenderMesh(node.mesh);
  }
  for (size_t c_idx = 0; c_idx < node.children.size(); ++c_idx) {
    RenderNode(model_.nodes[node.children[c_idx]], cur_transform);
  }
}

void Model::RenderMesh(int mesh_idx) {
  const auto &mesh = model_.meshes[mesh_idx];
  for (size_t p_idx = 0; p_idx < mesh.primitives.size(); ++p_idx) {
    const auto &primitive = mesh.primitives[p_idx];
    const auto &render_params = mesh_render_params_[mesh_idx][p_idx];
    glBindVertexArray(render_params.vao);

    int mode = -1;
    if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
      mode = GL_TRIANGLES;
    } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
      mode = GL_TRIANGLE_STRIP;
    } else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
      mode = GL_TRIANGLE_FAN;
    } else if (primitive.mode == TINYGLTF_MODE_POINTS) {
      mode = GL_POINTS;
    } else if (primitive.mode == TINYGLTF_MODE_LINE) {
      mode = GL_LINES;
    } else if (primitive.mode == TINYGLTF_MODE_LINE_LOOP) {
      mode = GL_LINE_LOOP;
    } else {
      CHECK(0) << "primitive.mode is invalid!";
    }
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    if (render_params.texture_id) {
      // Enable texture sampler.
      
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, render_params.texture_id);

      shader_->Set("has_texture", true);
      shader_->Set("diffuse_texture", 0);
    }
    else {
      shader_->Set("has_texture", false);
      shader_->Set("vertex_color", render_params.color);
    } 
    
    if (render_params.draw_type == DRAW_ARRAY) {
      glDrawArrays(mode, 0, render_params.count);
    } else if (render_params.draw_type == DRAW_ELEMENT) {
      const auto &index_accessor = model_.accessors[primitive.indices];
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
                   gpu_buffer_views_[index_accessor.bufferView]);
      glDrawElements(mode, render_params.count, index_accessor.componentType,
                     (GLvoid *)(index_accessor.byteOffset));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindVertexArray(0);
  }
}

GLuint Model::ProcessBufferView(const tinygltf::Accessor &accessor,
                                GLenum buffer_type) {
  const auto &buffer_view = model_.bufferViews[accessor.bufferView];
  const auto &buffer = model_.buffers[buffer_view.buffer];
  GLuint vbo = 0;
  glGenBuffers(1, &vbo);
  glBindBuffer(buffer_type, vbo);
  gpu_buffer_views_[accessor.bufferView] = vbo;

  if (!accessor.sparse.isSparse) {
    // parse raw data.
    glBufferData(buffer_type, buffer_view.byteLength,
                 &buffer.data.at(0) + buffer_view.byteOffset, GL_STATIC_DRAW);
  } else {
    // parse sparse data.
    std::vector<uint8_t> sparse_data(buffer_view.byteLength, 0);
    std::copy(buffer.data.data() + buffer_view.byteOffset,
              buffer.data.data() + buffer_view.byteOffset +
                  buffer_view.byteLength,
              sparse_data.data());

    const size_t size_of_object_in_buffer =
        GLTFComponentByteSize(accessor.componentType);
    const size_t size_of_sparse_indices =
        GLTFComponentByteSize(accessor.sparse.indices.componentType);
    const auto &indices_buffer_view =
        model_.bufferViews[accessor.sparse.indices.bufferView];
    const auto &indices_buffer = model_.buffers[indices_buffer_view.buffer];
    const auto &values_buffer_view =
        model_.bufferViews[accessor.sparse.values.bufferView];
    const auto &values_buffer = model_.buffers[values_buffer_view.buffer];

    for (size_t sparse_idx = 0; sparse_idx < accessor.sparse.count;
         ++sparse_idx) {
      int index = 0;
      switch (accessor.sparse.indices.componentType) {
      case TINYGLTF_COMPONENT_TYPE_BYTE:
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        index = (int)*(unsigned char *)(indices_buffer.data.data() +
                                        indices_buffer_view.byteOffset +
                                        accessor.sparse.indices.byteOffset +
                                        (sparse_idx * size_of_sparse_indices));
        break;
      case TINYGLTF_COMPONENT_TYPE_SHORT:
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        index = (int)*(unsigned short *)(indices_buffer.data.data() +
                                         indices_buffer_view.byteOffset +
                                         accessor.sparse.indices.byteOffset +
                                         (sparse_idx * size_of_sparse_indices));
        break;
      case TINYGLTF_COMPONENT_TYPE_INT:
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        index = (int)*(unsigned int *)(indices_buffer.data.data() +
                                       indices_buffer_view.byteOffset +
                                       accessor.sparse.indices.byteOffset +
                                       (sparse_idx * size_of_sparse_indices));
        break;
      }
      const uint8_t *read_from =
          values_buffer.data.data() +
          (values_buffer_view.byteOffset + accessor.sparse.values.byteOffset) +
          (sparse_idx * (size_of_object_in_buffer * accessor.type));

      uint8_t *write_to = sparse_data.data() +
                          index * (size_of_object_in_buffer * accessor.type);
      memcpy(write_to, read_from, size_of_object_in_buffer * accessor.type);
    }
    glBufferData(buffer_view.target, buffer_view.byteLength, sparse_data.data(),
                 GL_STATIC_DRAW);
  }
  glBindBuffer(buffer_type, 0);

  return vbo;
}