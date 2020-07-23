#include <algorithm>

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "common/logging.h"
#include "graphic/model.h"

void Model::Init(const std::string &model_path) {
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

  if (model_.textures.size() > 0) {
    if (model_.skins.size() > 0) {
      is_skinning_ = true;
      shader_.InitFromFile("../shader/avatar_tex_skin_vs.glsl",
                           "../shader/avatar_tex_skin_fs.glsl");
    } else {
      is_skinning_ = false;
      shader_.InitFromFile("../shader/avatar_tex_vs.glsl",
                           "../shader/avatar_tex_fs.glsl");
    }
  } else {
    if (model_.skins.size() > 0) {
      is_skinning_ = true;
      shader_.InitFromFile("../shader/avatar_notex_skin_vs.glsl",
                           "../shader/avatar_notex_skin_fs.glsl");
    } else {
      is_skinning_ = false;
      shader_.InitFromFile("../shader/avatar_notex_vs.glsl",
                           "../shader/avatar_notex_fs.glsl");
    }
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
        int size = GLTFTypeElmSize(accessor.type);
        int byte_stride =
            accessor.ByteStride(model_.bufferViews[accessor.bufferView]);
        CHECK(byte_stride != -1) << "byte_strid equal -1";
        if (a_iter->first == "POSITION") {
          cur_render_params.count = accessor.count;
          glBindBuffer(GL_ARRAY_BUFFER, cur_vbo);
          glVertexAttribPointer(0, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(0);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (a_iter->first == "TEXCOORD_0") {
          glBindBuffer(GL_ARRAY_BUFFER, cur_vbo);
          glVertexAttribPointer(1, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(1);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (a_iter->first == "NORMAL") {
          glBindBuffer(GL_ARRAY_BUFFER, cur_vbo);
          glVertexAttribPointer(2, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(2);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (a_iter->first == "JOINTS_0") {
          glBindBuffer(GL_ARRAY_BUFFER, cur_vbo);
          glVertexAttribPointer(3, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(3);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
        } else if (a_iter->first == "WEIGHTS_0") {
          glBindBuffer(GL_ARRAY_BUFFER, cur_vbo);
          glVertexAttribPointer(4, size, accessor.componentType,
                                accessor.normalized ? GL_TRUE : GL_FALSE,
                                byte_stride, (GLvoid *)(accessor.byteOffset));
          glEnableVertexAttribArray(4);
          glBindBuffer(GL_ARRAY_BUFFER, 0);
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

          glGenTextures(1, &cur_render_params.texture_id);
          glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
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
          } else {
            cur_render_params.color = glm::vec4(0.5, 0.5, 0.5, 1.0);
          }
        }
      }

      mesh_render_params_[m_idx].push_back(cur_render_params);
      glBindVertexArray(0);
    }
  }
  // build up skeleton
  scene_tree_.Init(model_);
}

void Model::Render(const glm::mat4 &view_matrix, const glm::mat4 &proj_matrix,
                   const glm::mat4 &model_matrix) {

  shader_.Use();
  // Set model matrix when render mesh
  shader_.Set("view_matrix", view_matrix);
  shader_.Set("proj_matrix", proj_matrix);

  // Suppose only one skin exists.
  scene_tree_.SetAnimationFrame(model_, GetTimeStampSecond());
  scene_tree_.UpdateGlobalPose();
  if (is_skinning_) {
    std::vector<float> skinning_pose_data;
    scene_tree_.GetSkinningPoseData(model_.skins[0].joints, skinning_pose_data);
    shader_.SetMat4Array("skinning_transforms", skinning_pose_data,
                         model_.skins[0].joints.size());
  }

  GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
  GLboolean last_enable_multisample = glIsEnabled(GL_MULTISAMPLE);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  int scene_to_display = model_.defaultScene > -1 ? model_.defaultScene : 0;
  const tinygltf::Scene &scene = model_.scenes[scene_to_display];
  for (size_t n_idx = 0; n_idx < scene.nodes.size(); ++n_idx) {
    RenderNode(scene.nodes[n_idx], model_matrix);
  }

  if (!last_enable_depth_test)
    glDisable(GL_DEPTH_TEST);
  if (!last_enable_multisample)
    glDisable(GL_MULTISAMPLE);
}

glm::mat4 GetNodeTransform(const tinygltf::Node &node) {
  glm::mat4 node_transform(1.f);
  if (node.matrix.size() == 16) {
    std::copy(node.matrix.begin(), node.matrix.end(), &node_transform[0][0]);
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
    node_transform = cur_trans * cur_rot * cur_scale;
  }

  return node_transform;
}

void QTSToMatrix(const std::vector<float> &qts, Eigen::Matrix4f &matrix) {
  Eigen::Matrix3f rotation = Eigen::Quaternion<float>(&qts[0]).matrix();
  Eigen::Matrix4f scale = Eigen::Matrix4f::Identity();
  scale(0, 0) = qts[7];
  scale(1, 1) = qts[7];
  scale(2, 2) = qts[7];

  matrix = Eigen::Matrix4f::Identity();
  matrix.block(0, 0, 3, 3) = rotation;

  matrix = scale * matrix;
  matrix.block(0, 3, 3, 1) << qts[4], qts[5], qts[6];
}

void MatrixToQTS(const Eigen::Matrix4f &matrix, std::vector<float> &qts) {
  Eigen::Matrix4f mat = matrix;

  qts = std::vector<float>(8, 0);
  Eigen::Matrix<float, 1, 3> x_vec, y_vec, z_vec;
  x_vec << mat(0, 0), mat(0, 1), mat(0, 2);
  y_vec << mat(1, 0), mat(1, 1), mat(1, 2);
  z_vec << mat(2, 0), mat(2, 1), mat(2, 2);

  float scale_x = x_vec.norm();
  float scale_y = y_vec.norm();
  float scale_z = z_vec.norm();
  x_vec.normalize();
  y_vec.normalize();
  z_vec.normalize();
  mat.block(0, 0, 1, 3) = x_vec;
  mat.block(1, 0, 1, 3) = y_vec;
  mat.block(2, 0, 1, 3) = z_vec;

  Eigen::Matrix3f rot_mat = mat.block(0, 0, 3, 3);

  Eigen::Quaternion<float> qua(rot_mat);
  qts[0] = qua.x();
  qts[1] = qua.y();
  qts[2] = qua.z();
  qts[3] = qua.w();
  qts[4] = mat(0, 3);
  qts[5] = mat(1, 3);
  qts[6] = mat(2, 3);
  qts[7] = (scale_x + scale_y + scale_z) * 0.3333333333f;
}

void Model::RenderNode(int node_idx, const glm::mat4 &parent_transform) {
  const auto &node = model_.nodes[node_idx];
  const Eigen::Matrix4f &node_local = scene_tree_.GetNode(node_idx)->local_mat_;
  glm::mat4 cur_transform(1.f);
  std::copy(node_local.data(), node_local.data() + node_local.size(),
            &cur_transform[0][0]);
  cur_transform = parent_transform * cur_transform;
  if (node.mesh > -1) {
    // Set model matrix.
    shader_.Set("model_matrix", cur_transform);
    RenderMesh(node.mesh);
  }
  for (size_t c_idx = 0; c_idx < node.children.size(); ++c_idx) {
    RenderNode(node.children[c_idx], cur_transform);
  }
}

void Model::RenderMesh(int mesh_idx) {
  const auto &mesh = model_.meshes[mesh_idx];
  for (size_t p_idx = 0; p_idx < mesh.primitives.size(); ++p_idx) {
    const auto &primitive = mesh.primitives[p_idx];
    const auto &render_params = mesh_render_params_[mesh_idx][p_idx];
    glBindVertexArray(render_params.vao);

    int mode = GLTFRenderMode(primitive.mode);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    if (is_skinning_) {
      glEnableVertexAttribArray(3);
      glEnableVertexAttribArray(4);
    }

    if (render_params.texture_id) {
      // Enable texture sampler.
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, render_params.texture_id);
      shader_.Set("diffuse_texture", 0);

    } else {
      shader_.Set("vertex_color", render_params.color);
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
    if (is_skinning_) {
      glDisableVertexAttribArray(3);
      glDisableVertexAttribArray(4);
    }
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

void SceneTree::Init(const tinygltf::Model &model) {
  node_array_.clear();
  node_name2index_map_.clear();
  for (size_t n_idx = 0; n_idx < model.nodes.size(); ++n_idx) {
    const auto &node = model.nodes[n_idx];
    glm::mat4 node_transform = GetNodeTransform(node);
    node_array_.push_back(STLMakeSharedOfEigenTypes<SceneTreeNode>(
        n_idx, -1, node.name, Eigen::Matrix4f(&node_transform[0][0]),
        Eigen::Matrix4f::Identity(), Eigen::Matrix4f::Identity()));
    node_name2index_map_[node.name] = n_idx;
  }
  for (size_t n_idx = 0; n_idx < model.nodes.size(); ++n_idx) {
    const auto &node = model.nodes[n_idx];
    for (const auto &child_idx : node.children) {
      node_array_[child_idx]->parent_idx_ = n_idx;
    }
  }
  BuildGraph();
  // only update inv_bind_mat_ when initing.
  UpdateGlobalPose(true);
}

SceneTree::SceneTree(const std::vector<SceneTreeNode::Ptr> &input_bone_array)
    : node_array_(input_bone_array) {
  for (auto bone_ptr : node_array_) {
    node_name2index_map_[bone_ptr->name_] = bone_ptr->idx_;
  }
  BuildGraph();
  UpdateGlobalPose();
}

SceneTree SceneTree::Copy() const {
  std::vector<SceneTreeNode::Ptr> new_bone_array;
  for (const auto &cur_bone_ptr : node_array_) {
    new_bone_array.push_back(STLMakeSharedOfEigenTypes<SceneTreeNode>(
        cur_bone_ptr->idx_, cur_bone_ptr->parent_idx_, cur_bone_ptr->name_,
        cur_bone_ptr->local_mat_, cur_bone_ptr->global_mat_,
        cur_bone_ptr->inv_bind_mat_));
  }
  return SceneTree(new_bone_array);
}

glm::vec3 SceneTree::GetRootTrans() const {
  return glm::vec3(root_->left_node_->global_mat_(0, 3),
                   root_->left_node_->global_mat_(1, 3),
                   root_->left_node_->global_mat_(2, 3));
}

void SceneTree::BuildGraph() {
  CHECK(!this->graph_inited_) << "graph is already initialized.";
  root_ = STLMakeSharedOfEigenTypes<SceneTreeNode>(
      -1, -1, "", Eigen::Matrix4f::Identity(), Eigen::Matrix4f::Identity(),
      Eigen::Matrix4f::Identity());
  for (auto i = 0; i < this->node_array_.size(); i++) {
    auto &cur_bone_ptr = this->node_array_[i];
    SceneTreeNode::Ptr parent_ptr;
    if (cur_bone_ptr->parent_idx_ == -1) {
      parent_ptr = root_;
    } else {
      parent_ptr = this->node_array_[cur_bone_ptr->parent_idx_];
    }
    if (parent_ptr->left_node_ == nullptr) {
      parent_ptr->left_node_ = cur_bone_ptr;
    } else {
      cur_bone_ptr->right_node_ = parent_ptr->left_node_;
      parent_ptr->left_node_ = cur_bone_ptr;
    }
  }
  this->graph_inited_ = true;
}

void SceneTree::UpdateGlobalPose(bool with_inverse) {
  // Use depth first search to update global matrix.
  STLVectorOfEigenTypes<std::pair<SceneTreeNode::Ptr, Eigen::Matrix4f>> s;
  s.push_back(std::make_pair(root_->left_node_, Eigen::Matrix4f::Identity()));

  while (!s.empty()) {
    SceneTreeNode::Ptr cur_node = s.back().first;
    Eigen::Matrix4f global_transfrom = s.back().second;
    s.pop_back();

    // push brother node.
    if (cur_node->right_node_ != nullptr) {
      s.push_back(std::make_pair(cur_node->right_node_, global_transfrom));
    }
    Eigen::Matrix4f local_mat = cur_node->local_mat_;
    global_transfrom = global_transfrom * local_mat;
    cur_node->global_mat_ = global_transfrom;
    if (with_inverse) {
      cur_node->inv_bind_mat_ = cur_node->global_mat_.inverse();
    }
    if (cur_node->left_node_ != nullptr) {
      s.push_back(std::make_pair(cur_node->left_node_, global_transfrom));
    }
  }
}

void SceneTree::GetSkinningPoseData(const std::vector<int> &joints_used,
                                    std::vector<float> &skinning_pose_data) {
  skinning_pose_data.resize(joints_used.size() * 16, 0);
  for (int i = 0; i < joints_used.size(); ++i) {
    int b_idx = joints_used[i];
    Eigen::Matrix4f pose_mat =
        node_array_[b_idx]->global_mat_ * node_array_[b_idx]->inv_bind_mat_;
    std::copy(pose_mat.data(), pose_mat.data() + 16,
              &skinning_pose_data[16 * i]);
  }
}

void SceneTree::SetAnimationFrame(const tinygltf::Model &model,
                                  double time_stamp) {
  if (anim_time_ < 0) {
    anim_timestamp_ = time_stamp;
  }
  anim_time_ = time_stamp - anim_timestamp_;

  // no animation information.
  for (int anim_idx = 0; anim_idx < model.animations.size(); ++anim_idx) {
    const auto &animation_samplers = model.animations[anim_idx].samplers;
    const auto &animation_channels = model.animations[anim_idx].channels;

    for (int chan_idx = 0; chan_idx < animation_channels.size(); ++chan_idx) {
      const auto &cur_anim_channel = animation_channels[chan_idx];
      const auto &cur_anim_sampler =
          animation_samplers[cur_anim_channel.sampler];

      // Currently only support rotation animation.
      // Cause I only used this program for rigid body.
      int node_idx = cur_anim_channel.target_node;
      Eigen::Matrix4f cur_local = node_array_[node_idx]->local_mat_;
      std::vector<float> qts_array;
      MatrixToQTS(cur_local, qts_array);

      const auto &time_accessor = model.accessors[cur_anim_sampler.input];
      const auto &value_accessor = model.accessors[cur_anim_sampler.output];

      const auto &time_buffer_view =
          model.bufferViews[time_accessor.bufferView];
      const auto &time_buffer = model.buffers[time_buffer_view.buffer];

      const float *time_ptr = reinterpret_cast<const float *>(
          time_buffer.data.data() + time_accessor.byteOffset);
      int time_elm_size = GLTFTypeElmSize(time_accessor.type);
      std::vector<float> time_arr(
          time_ptr,
          time_accessor.count * time_elm_size + time_ptr);

      double max_time = *time_arr.rbegin();

      const auto &value_buffer_view =
          model.bufferViews[value_accessor.bufferView];
      const auto &value_buffer = model.buffers[value_buffer_view.buffer];
      const float *value_ptr = reinterpret_cast<const float *>(
          value_buffer.data.data() + value_accessor.byteOffset);
      int value_elm_size = GLTFTypeElmSize(value_accessor.type);
      std::vector<float> value_arr(value_ptr, value_ptr + value_accessor.count *
                                                              value_elm_size);
      double cur_mod_time = GetMod(anim_time_, max_time);
      auto time_iter = std::lower_bound(time_arr.begin(), time_arr.end(), cur_mod_time);
      int index = time_iter - time_arr.begin() - 1;
      if (index < 0) index = 0;

      float weight = (cur_mod_time - time_arr[index]) / (time_arr[index + 1] - time_arr[index]);

      // Interpolate the animation.
      if (cur_anim_channel.target_path == "rotation") {
        // find the left 
        glm::quat quat_left(value_arr[4 * index + 3], value_arr[4 * index + 0], value_arr[4 * index + 1], value_arr[4 * index + 2]);
        glm::quat quat_right(value_arr[4 * (index + 1) + 3], value_arr[4 * (index + 1) + 0], value_arr[4 * (index + 1) + 1], value_arr[4 * (index + 1) + 2]);
        glm::quat quat_slerp = glm::shortMix(quat_left, quat_right, weight);
        qts_array[0] = quat_slerp.x;
        qts_array[1] = quat_slerp.y;
        qts_array[2] = quat_slerp.z;
        qts_array[3] = quat_slerp.w;
      } else if (cur_anim_channel.target_path == "translation") {
        qts_array[4] = (1 - weight) * value_arr[3 * index + 0] + weight * value_arr[3 * (index + 1) + 0];
        qts_array[5] = (1 - weight) * value_arr[3 * index + 1] + weight * value_arr[3 * (index + 1) + 1];
        qts_array[6] = (1 - weight) * value_arr[3 * index + 2] + weight * value_arr[3 * (index + 1) + 2];
      } else if (cur_anim_channel.target_path == "scale") {
        //float s0 = (1 - weight) * value_arr[3 * index + 0] + weight * value_arr[3 * (index + 1) + 0];
        //float s1 = (1 - weight) * value_arr[3 * index + 1] + weight * value_arr[3 * (index + 1) + 1];
        //float s2 = (1 - weight) * value_arr[3 * index + 2] + weight * value_arr[3 * (index + 1) + 2];
        //qts_array[7] = (s0 + s1 + s2) / 3.0;
        //LOG(INFO) << qts_array[7];
      }
      QTSToMatrix(qts_array, node_array_[node_idx]->local_mat_);
    }
  }
}

void SceneTree::SetLocalPose(const std::vector<float> &transform_array) {
  CHECK(!node_array_.empty()) << "Bonemap is not inited!";
  int mat_size = node_array_[0]->local_mat_.size();
  CHECK(transform_array.size() == node_array_.size() * mat_size)
      << "transform_array size is invalid: " << transform_array.size()
      << "(wish " << node_array_.size() * mat_size << ").";

  for (int b_idx = 0; b_idx < node_array_.size(); ++b_idx) {
    node_array_[b_idx]->local_mat_ =
        Eigen::Map<const Eigen::Matrix4f>(&transform_array[b_idx * mat_size]);
  }
}

void SceneTree::GetGlobalKeypoints(
    const std::vector<std::string> &keypoint_names,
    std::vector<glm::vec3> &keypoints) {
  keypoints.clear();
  UpdateGlobalPose();
  for (auto cur_name : keypoint_names) {
    auto cur_bone = node_array_[node_name2index_map_[cur_name]];
    keypoints.push_back(glm::vec3(cur_bone->global_mat_(0, 3),
                                  cur_bone->global_mat_(1, 3),
                                  cur_bone->global_mat_(2, 3)));
  }
}

SceneTree SceneTree::Split(
    const std::vector<std::string> &scene_tree_ordered_name) const {
  std::vector<bool> scene_tree_valid(this->node_array_.size(), false);
  std::map<int, int> oldIdx2newIdx_map;
  std::vector<SceneTreeNode::Ptr> new_bone_array(
      scene_tree_ordered_name.size());

  for (auto i = 0; i < scene_tree_ordered_name.size(); i++) {
    auto bone_name = scene_tree_ordered_name[i];
    CHECK(this->node_name2index_map_.find(bone_name) !=
          this->node_name2index_map_.end())
        << "can't find bone " << bone_name;
    auto cur_idx = this->node_name2index_map_.at(bone_name);
    scene_tree_valid[cur_idx] = true;
    oldIdx2newIdx_map[cur_idx] = i;
  }
  for (auto i = 0; i < this->node_array_.size(); i++) {
    auto bone_ptr = this->node_array_[i];
    if (!scene_tree_valid[i])
      continue;

    auto new_idx = oldIdx2newIdx_map[i];
    auto old_parent_idx = bone_ptr->parent_idx_;
    Eigen::Matrix4f local_mat = bone_ptr->local_mat_;
    int new_parent_idx = -1;
    while (old_parent_idx != -1) {
      auto parent_ptr = this->node_array_[old_parent_idx];
      if (scene_tree_valid[old_parent_idx]) {
        new_parent_idx = oldIdx2newIdx_map[old_parent_idx];
        break;
      } else {
        local_mat = parent_ptr->local_mat_ * local_mat;
        old_parent_idx = parent_ptr->parent_idx_;
      }
    }
    new_parent_idx =
        (old_parent_idx == -1) ? -1 : oldIdx2newIdx_map[old_parent_idx];

    new_bone_array[new_idx] = STLMakeSharedOfEigenTypes<SceneTreeNode>(
        new_idx, new_parent_idx, this->node_array_[i]->name_, local_mat,
        this->node_array_[i]->global_mat_, this->node_array_[i]->inv_bind_mat_);
  }
  SceneTree result(new_bone_array);
  result.UpdateGlobalPose();
  return result;
}

SceneTree SceneTree::UpdateTransform(
    const std::vector<std::string> &ordered_names,
    const STLVectorOfEigenTypes<Eigen::Matrix4f> &transforms) const {
  CHECK(ordered_names.size() == transforms.size())
      << ordered_names.size() << ", " << transforms.size();
  std::vector<SceneTreeNode::Ptr> new_bone_array(this->node_array_.size(),
                                                 nullptr);

  for (auto i = 0; i < new_bone_array.size(); i++) {
    new_bone_array[i] = STLMakeSharedOfEigenTypes<SceneTreeNode>(
        this->node_array_[i]->idx_, this->node_array_[i]->parent_idx_,
        this->node_array_[i]->name_, this->node_array_[i]->local_mat_,
        this->node_array_[i]->global_mat_, this->node_array_[i]->inv_bind_mat_);
  }
  for (auto i = 0; i < ordered_names.size(); i++) {
    auto bone_name = ordered_names[i];
    auto transform = transforms[i];
    auto iter = this->node_name2index_map_.find(bone_name);
    CHECK(iter != this->node_name2index_map_.end())
        << "can't find bone " << bone_name;
    auto cur_idx = iter->second;
    new_bone_array[cur_idx]->local_mat_ =
        new_bone_array[cur_idx]->local_mat_ * transform;
  }
  SceneTree result(new_bone_array);
  result.UpdateGlobalPose();
  return result;
}

void SceneTree::SetNodeTranslation(const glm::vec3 &translation,
                                   const std::string &node_name) {
  node_array_[this->node_name2index_map_.at(node_name)]->local_mat_.block(0, 3,
                                                                          3, 1)
      << translation.x,
      translation.y, translation.z;
}

void SceneTree::AddFakedBoneNode(
    const std::vector<std::string> &bone_names,
    const std::vector<std::string> &parent_names,
    const STLVectorOfEigenTypes<Eigen::Matrix4f> &rest_mats) {
  int cur_offset = this->node_array_.size();
  for (auto i = 0; i < bone_names.size(); i++) {
    auto iter = this->node_name2index_map_.find(parent_names[i]);
    CHECK(iter != this->node_name2index_map_.end())
        << "can't find bone " << parent_names[i];
    this->node_name2index_map_[bone_names[i]] = cur_offset;
    SceneTreeNode::Ptr parent_ptr = this->node_array_[iter->second];

    Eigen::Matrix4f global_mat = parent_ptr->global_mat_ * rest_mats[i];
    SceneTreeNode::Ptr new_bone_ptr = STLMakeSharedOfEigenTypes<SceneTreeNode>(
        cur_offset++, iter->second, bone_names[i], rest_mats[i], global_mat,
        global_mat.inverse());

    if (parent_ptr->left_node_ == nullptr) {
      parent_ptr->left_node_ = new_bone_ptr;
    } else {
      new_bone_ptr->right_node_ = parent_ptr->left_node_;
      parent_ptr->left_node_ = new_bone_ptr;
    }
    this->node_array_.emplace_back(new_bone_ptr);
  }
}

void SceneTree::GetLocalTransform(std::vector<float> &transform_array) {
  CHECK(!node_array_.empty()) << "Bonemap hasn't been inited!";
  int mat_size = node_array_[0]->local_mat_.size();
  std::vector<float> cur_transform_array(mat_size * node_array_.size());

  for (int b_idx = 0; b_idx < node_array_.size(); ++b_idx) {
    std::copy(node_array_[b_idx]->local_mat_.data(),
              node_array_[b_idx]->local_mat_.data() +
                  node_array_[b_idx]->local_mat_.size(),
              &cur_transform_array[mat_size * b_idx]);
  }
  transform_array = cur_transform_array;
}

void SceneTree::SetBoneTranslation(const std::string &bone_name,
                                   const glm::vec3 &translation,
                                   std::vector<float> &transform_array) {
  auto bone_iter = node_name2index_map_.find(bone_name);
  CHECK(bone_iter != node_name2index_map_.end())
      << "Bonename : " << bone_name << " doesn't exist in bone_array!";
  Eigen::Matrix4f cur_transform_matrix(
      &transform_array[16 * bone_iter->second]);
  cur_transform_matrix.block(0, 3, 3, 1) << translation.x, translation.y,
      translation.z;
  std::copy(cur_transform_matrix.data(),
            cur_transform_matrix.data() + cur_transform_matrix.size(),
            &transform_array[16 * bone_iter->second]);
}

void SceneTree::ConvertLocalRotationToGlobalRotation(
    const STLVectorOfEigenTypes<Eigen::Matrix4f> &local_rotation_mats,
    STLVectorOfEigenTypes<Eigen::Matrix4f> &global_rotation_mats) {
  CHECK(local_rotation_mats.size() == node_array_.size())
      << "local_rotation_mats size(" << local_rotation_mats.size()
      << ") doesn't match bone_array size(" << node_array_.size() << ")";
  global_rotation_mats.resize(local_rotation_mats.size(),
                              Eigen::Matrix4f::Identity());
  for (int b_idx = 0; b_idx < node_array_.size(); ++b_idx) {
    // Remove the offset, and convert local rotation to global rotaiton.
    Eigen::Matrix4f global_mat = Eigen::Matrix4f::Identity();
    global_mat.block(0, 0, 3, 3) =
        node_array_[b_idx]->global_mat_.block(0, 0, 3, 3);
    // local_rotation_mats is use on local_vec, to make it global,
    // I need use local_vec = global_mat.inverse() * global_vec
    // So global rotation (use on global vec and under global coordinate) is
    // global_mat * local_rotation_mats[b_idx] * global_mat.inverse().
    global_rotation_mats[b_idx] =
        global_mat * local_rotation_mats[b_idx] * global_mat.inverse();
  }
}

int GLTFTypeElmSize(int type) {
  if (type == TINYGLTF_TYPE_SCALAR) {
    return 1;
  } else if (type == TINYGLTF_TYPE_VEC2) {
    return 2;
  } else if (type == TINYGLTF_TYPE_VEC3) {
    return 3;
  } else if (type == TINYGLTF_TYPE_VEC4) {
    return 4;
  } else {
    CHECK(0) << "Don't support accessor.type: " << type;
  }
  return 0;
}
size_t GLTFComponentByteSize(int type) {
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

GLenum GLTFRenderMode(int mode) {
  if (mode == TINYGLTF_MODE_TRIANGLES) {
    return GL_TRIANGLES;
  } else if (mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
    return GL_TRIANGLE_STRIP;
  } else if (mode == TINYGLTF_MODE_TRIANGLE_FAN) {
    return GL_TRIANGLE_FAN;
  } else if (mode == TINYGLTF_MODE_POINTS) {
    return GL_POINTS;
  } else if (mode == TINYGLTF_MODE_LINE) {
    return GL_LINES;
  } else if (mode == TINYGLTF_MODE_LINE_LOOP) {
    return GL_LINE_LOOP;
  } else {
    CHECK(0) << "primitive.mode is invalid!";
  }
  return GL_TRIANGLES;
}