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
size_t GLTFComponentByteSize(int type);
int GLTFTypeElmSize(int type);
GLenum GLTFRenderMode(int mode);
glm::mat4 GetNodeTransform(const tinygltf::Node& node);
void QTSToMatrix(const std::vector<float>& qts,
  Eigen::Matrix4f& matrix);
void MatrixToQTS(const Eigen::Matrix4f& _matrix,
  std::vector<float>& qts);

// Define the skeleton 
class SceneTreeNode {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  using Ptr = std::shared_ptr<SceneTreeNode>;

  SceneTreeNode() = delete;
  SceneTreeNode(int index, int parent_idx, const std::string& name,
                const Eigen::Matrix4f& local_mat,
                const Eigen::Matrix4f& global_mat,
                const Eigen::Matrix4f& inv_bind_mat)
      : idx_(index),
        parent_idx_(parent_idx),
        name_(name),
        local_mat_(local_mat),
        global_mat_(global_mat),
        inv_bind_mat_(inv_bind_mat){}
  ~SceneTreeNode() = default;

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

class SceneTree {
 public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  SceneTree() = default;
  ~SceneTree() = default;

  void Init(const tinygltf::Model& model);

  SceneTree Copy() const;

  void UpdateGlobalPose(bool with_inverse = false);
  // Before get, you may need UpdateGlobalPose first if the local pose has been changed.
  void GetSkinningPoseData(const std::vector<int>& joints_used, std::vector<float>& skinning_pose_data);
  void SetAnimationFrame(const tinygltf::Model& model, double time_stamp);
  void ResetAnimationTimer() {
    anim_time_ = 0;
    anim_timestamp_ = -1;
  }

  void SetLocalPose(const std::vector<float>& transform_array);

  void GetGlobalKeypoints(const std::vector<std::string>& keypoint_names,
                          std::vector<glm::vec3>& keypoints);

  // split current skeleton into smaller one
  SceneTree Split(
      const std::vector<std::string>& skeleton_ordered_name) const;

  SceneTree UpdateTransform(
      const std::vector<std::string>& ordered_names,
      const STLVectorOfEigenTypes<Eigen::Matrix4f>& transforms) const;

  void SetNodeTranslation(const glm::vec3& translation,
                          const std::string& node_names);

  int GetNodeNum() const { return node_array_.size(); }

  SceneTreeNode::Ptr GetNode(const std::string& node_name) {
    if (node_name2index_map_.find(node_name) != node_name2index_map_.end()) {
      return GetNode(node_name2index_map_[node_name]);
    }
    else {
      LOG(WARNING) << "Node: " << node_name << " doesn't exist in the node array";
      return nullptr;
    }
  }
  SceneTreeNode::Ptr GetNode(int node_idx) {
    return node_array_[node_idx];
  }


  glm::vec3 GetRootTrans() const;

  // recover pose by eular angle.
  template <typename T>
  void RecoverPose(Eigen::Matrix<T, Eigen::Dynamic, 3>& result_pose,
                   const std::vector<T>& params = {},
                   const std::vector<Eigen::Vector3d>& bone_transforms = {},
                   bool use_xyz = true) const {
    STLVectorOfEigenTypes<std::pair<SceneTreeNode::Ptr, Eigen::Matrix<T, 4, 4>>>
        queue;
    queue.push_back(
        std::make_pair(root_->left_node_, Eigen::Matrix<T, 4, 4>::Identity()));
    while (!queue.empty()) {
      SceneTreeNode::Ptr cur_node = queue.back().first;
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

 private:
  explicit SceneTree(
      const std::vector<SceneTreeNode::Ptr>& bone_array);

  std::vector<SceneTreeNode::Ptr> node_array_;
  std::map<std::string, int> node_name2index_map_;
  SceneTreeNode::Ptr root_;
  bool graph_inited_ = false;
  void BuildGraph();

  double anim_time_ = -1;
  double anim_timestamp_ = 0;
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

  void RenderNode(int node_idx, const glm::mat4& parent_transform);
  void RenderMesh(int mesh_idx);
  GLuint ProcessBufferView(const tinygltf::Accessor& accessor, GLenum buffer_type);
  tinygltf::Model model_;

  bool is_skinning_ = false;
  std::vector<int> skinning_joints_;
  STLVectorOfEigenTypes<Eigen::Matrix4f> skinning_invbindmat_;
  SceneTree scene_tree_;

  Shader shader_;

  std::map<int, std::vector<RenderParams>> mesh_render_params_;
  std::map<int, GLuint> gpu_buffer_views_;
};