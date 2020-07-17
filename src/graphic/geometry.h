#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "common/logging.h"
#include "graphic/shader.h"

namespace Geometry {

class Base {
public:
  Base() = default;
  virtual ~Base() {}
  // Wrong implement, but test first.
  struct Face {
    // counter clock-wise
    Face(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2,
         glm::vec4 c = glm::vec4(0.5, 0.5, 0.5, 1.0)) {
      points = std::vector<glm::vec3>{p0, p1, p2};
      color = std::vector<glm::vec4>{c, c, c};
      glm::vec3 vec0 = p1 - p0;
      glm::vec3 vec1 = p2 - p1;
      glm::vec3 face_normal = glm::cross(vec0, vec1);
      face_normal = glm::normalize(face_normal);
      normal = std::vector<glm::vec3>{face_normal, face_normal, face_normal};
    }
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normal;
    std::vector<glm::vec4> color;
  };

protected:
  std::vector<Face> triangle_faces_;
};

class Grid : Base {
public:
  ~Grid() {}

  Grid() = default;
  void Init(float edge_size, float grid_size);

  void GetData(std::vector<float> &vertex_data, std::vector<float> &normal_data,
               std::vector<float> &color_data);

private:
  float edge_size_ = 0;
  float grid_size_ = 0;
};

} // namespace Geometry