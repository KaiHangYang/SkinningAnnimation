#include <GL/gl3w.h>

#include "graphic/geometry.h"

namespace Geometry {

void Grid::Init(float edge_size, float grid_size) {
  edge_size_ = edge_size;
  grid_size_ = grid_size;
  CHECK(edge_size_ > 0 && grid_size_ > 0)
      << "edge_size and grid_size must > 0!";
  glm::vec4 white_color(0.8, 0.8, 0.8, 1.0);
  glm::vec4 black_color(0.3, 0.3, 0.3, 1.0);

  bool row_flag = true;
  bool col_flag = true;
  glm::vec4 color;
  for (float x = -edge_size_ * 0.5f; x < edge_size_ * 0.5f; x += grid_size_) {
    if (row_flag) {
      col_flag = true;
    } else {
      col_flag = false;
    }
    row_flag = !row_flag;
    for (float z = -edge_size_ * 0.5f; z < edge_size_ * 0.5f; z += grid_size_) {
      if (col_flag) {
        color = white_color;
      } else {
        color = black_color;
      }
      col_flag = !col_flag;

      triangle_faces_.emplace_back(glm::vec3(x + grid_size_, 0, z + grid_size_),
                                   glm::vec3(x + grid_size_, 0, z),
                                   glm::vec3(x, 0, z), color);
      triangle_faces_.emplace_back(glm::vec3(x + grid_size_, 0, z + grid_size_),
                                   glm::vec3(x, 0, z),
                                   glm::vec3(x, 0, z + grid_size_), color);
    }
  }
} // namespace Geometry

void Grid::GetData(std::vector<float> &vertex_data,
                   std::vector<float> &normal_data,
                   std::vector<float> &color_data) {
  vertex_data.clear();
  normal_data.clear();
  color_data.clear();
  for (int f_idx = 0; f_idx < triangle_faces_.size(); ++f_idx) {
    const auto &cur_face = triangle_faces_[f_idx];
    for (int j_idx = 0; j_idx < cur_face.points.size(); ++j_idx) {
      vertex_data.push_back(cur_face.points[j_idx].x);
      vertex_data.push_back(cur_face.points[j_idx].y);
      vertex_data.push_back(cur_face.points[j_idx].z);
    }
    for (int j_idx = 0; j_idx < cur_face.normal.size(); ++j_idx) {
      normal_data.push_back(cur_face.normal[j_idx].x);
      normal_data.push_back(cur_face.normal[j_idx].y);
      normal_data.push_back(cur_face.normal[j_idx].z);
    }
    for (int j_idx = 0; j_idx < cur_face.color.size(); ++j_idx) {
      color_data.push_back(cur_face.color[j_idx].r);
      color_data.push_back(cur_face.color[j_idx].g);
      color_data.push_back(cur_face.color[j_idx].b);
      color_data.push_back(cur_face.color[j_idx].a);
    }
  }
}
}; // namespace Geometry
