#pragma once

#include <GL/gl3w.h>
#include <memory>
#include <string>
#include <tiny_gltf.h>
#include <vector>
#include <set>
#include <glm/glm.hpp>

#include "common/utility.h"
#include "graphic/shader.h"


// Helper function.
glm::mat4 GetNodeTransform(const tinygltf::Node& node);

// Define the skeleton 
class SkeletonNode {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  using Ptr = std::shared_ptr<SkeletonNode>;

  SkeletonNode() = delete;
  SkeletonNode(int index, int parent_idx, const std::string& name,
                const Eigen::Matrix4f& local_mat,
                const Eigen::Matrix4f& global_mat,
                const Eigen::Matrix4f& inv_bind_mat)
      : idx_(index),
        parent_idx_(parent_idx),
        name_(name),
        local_mat_(local_mat),
        global_mat_(global_mat),
        inv_bind_mat_(inv_bind_mat){}
  ~SkeletonNode() = default;

  int idx_;
  int parent_idx_;
  std::string name_;
  Eigen::Matrix4f local_mat_;
  Eigen::Matrix4f global_mat_;
  Eigen::Matrix4f inv_bind_mat_;

  // left node for first child.
  Ptr left_node_ = nullptr;
  // right node for brother.
  Ptr right_node_ = nullptr;
};

class Skeleton {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  Skeleton() = default;
  ~Skeleton() = default;

  void Init(const tinygltf::Model& model);

  Skeleton Copy() const;

  void UpdateGlobalPose(bool with_inverse = false);
  // Before get, you may need UpdateGlobalPose first if the local pose has been changed.
  void GetSkinningPoseData(const std::vector<int>& joints_used, std::vector<float>& skinning_pose_data);
  void SetLocalPose(const std::vector<float>& transform_array);

  void GetGlobalKeypoints(const std::vector<std::string>& keypoint_names,
                          std::vector<glm::vec3>& keypoints);

  // split current skeleton into smaller one
  Skeleton Split(
      const std::vector<std::string>& skeleton_ordered_name) const;

  Skeleton UpdateTransform(
      const std::vector<std::string>& ordered_names,
      const STLVectorOfEigenTypes<Eigen::Matrix4f>& transforms) const;

  void SetBoneTranslation(const glm::vec3& translation,
                          const std::string& bone_names);

  int GetBoneNum() const { return bone_array_.size(); }

  glm::vec3 GetRootTrans() const;

  // recover pose by eular angle.
  template <typename T>
  void RecoverPose(Eigen::Matrix<T, Eigen::Dynamic, 3>& result_pose,
                   const std::vector<T>& params = {},
                   const std::vector<Eigen::Vector3d>& bone_transforms = {},
                   bool use_xyz = true) const {
    STLVectorOfEigenTypes<std::pair<SkeletonNode::Ptr, Eigen::Matrix<T, 4, 4>>>
        queue;
    queue.push_back(
        std::make_pair(root_->left_node_, Eigen::Matrix<T, 4, 4>::Identity()));
    while (!queue.empty()) {
      SkeletonNode::Ptr cur_node = queue.back().first;
      Eigen::Matrix<T, 4, 4> global_transform = queue.back().second;
      queue.pop_back();

      if (cur_node->right_node_ != nullptr) {
        queue.push_back(
            std::make_pair(cur_node->right_node_, global_transform));
      }
      Eigen::Matrix<T, 4, 4> rest_mat = cur_node->local_mat_.template cast<T>();
      if (!bone_transforms.empty() && cur_node->name_ != "Root_M") {
        rest_mat.block(0, 3, 3, 1) << bone_transforms[cur_node->idx_].cast<T>();
      }
      global_transform = global_transform * rest_mat;
      if (cur_node->left_node_ != nullptr && !params.empty()) {
        Eigen::Matrix<T, 4, 4> local_transform =
            Eigen::Matrix<T, 4, 4>::Identity();
        Eigen::Matrix<T, 3, 3> cur_rotation =
            Eigen::Matrix<T, 3, 3>::Identity();
        if (use_xyz) {
          cur_rotation = Eigen::AngleAxis<T>(params[3 * cur_node->idx_ + 2],
                                             Eigen::Matrix<T, 3, 1>::UnitZ()) *
                         Eigen::AngleAxis<T>(params[3 * cur_node->idx_ + 1],
                                             Eigen::Matrix<T, 3, 1>::UnitY()) *
                         Eigen::AngleAxis<T>(params[3 * cur_node->idx_ + 0],
                                             Eigen::Matrix<T, 3, 1>::UnitX());
        } else {
          cur_rotation = Eigen::AngleAxis<T>(params[3 * cur_node->idx_ + 0],
                                             Eigen::Matrix<T, 3, 1>::UnitX()) *
                         Eigen::AngleAxis<T>(params[3 * cur_node->idx_ + 1],
                                             Eigen::Matrix<T, 3, 1>::UnitY()) *
                         Eigen::AngleAxis<T>(params[3 * cur_node->idx_ + 2],
                                             Eigen::Matrix<T, 3, 1>::UnitZ());
        }
        local_transform.block(0, 0, 3, 3) = cur_rotation;
        global_transform = global_transform * local_transform;
      }
      result_pose.row(cur_node->idx_) << global_transform(0, 3),
          global_transform(1, 3), global_transform(2, 3);
      if (cur_node->left_node_ != nullptr) {
        queue.push_back(std::make_pair(cur_node->left_node_, global_transform));
      }
    }
  }

  void AddFakedBoneNode(
      const std::vector<std::string>& bone_names,
      const std::vector<std::string>& parent_names,
      const STLVectorOfEigenTypes<Eigen::Matrix4f>& rest_mats);

  // only support entire skeleton
  void GetLocalTransform(std::vector<float>& transfrom_array);

  // calculate the rotation under global(Identity) coordinate.
  void SetBoneTranslation(const std::string& bone_name,
                          const glm::vec3& translation,
                          std::vector<float>& transform_array);
  void ConvertLocalRotationToGlobalRotation(
      const STLVectorOfEigenTypes<Eigen::Matrix4f>& local_rotation_mats,
      STLVectorOfEigenTypes<Eigen::Matrix4f>& global_rotation_mats);

  std::vector<SkeletonNode::Ptr> bone_array_;
  std::map<std::string, int> bone_name2index_map;

 private:
  explicit Skeleton(
      const std::vector<SkeletonNode::Ptr>& bone_array);

  SkeletonNode::Ptr root_;
  bool graph_inited = false;
  void BuildGraph();
};

class Model {
public:
  enum DrawType {DRAW_ARRAY = 0, DRAW_ELEMENT = 1};
  Model() = default;
  void Init(const std::string &model_path);
  void Render(const glm::mat4& view_matrix, const glm::mat4& proj_matrix, const glm::mat4& model_matrix);
  ~Model(){};

private:
  struct RenderParams {
    DrawType draw_type = DRAW_ARRAY;
    int count = 0;
    GLuint vao = 0;
    GLuint indices_vbo = 0;
    GLuint texture_id = 0;
    glm::vec4 color = glm::vec4(0.5, 0.5, 0.5, 1.0);
  };

  void RenderNode(const tinygltf::Node& node, const glm::mat4& parent_transform);
  void RenderMesh(int mesh_idx);
  GLuint ProcessBufferView(const tinygltf::Accessor& accessor, GLenum buffer_type);
  size_t GLTFComponentByteSize(int type);
  tinygltf::Model model_;

  bool is_skinning_ = false;
  Skeleton skinning_skeleton_;

  Shader shader_;

  std::map<int, std::vector<RenderParams>> mesh_render_params_;
  std::map<int, GLuint> gpu_buffer_views_;
};