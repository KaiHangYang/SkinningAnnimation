#pragma once

#include <GL/gl3w.h>
#include <opencv2/core/core.hpp>

class Texture {
public:
  explicit Texture();
  ~Texture();
  bool LoadImage(const cv::Mat &img);
  GLuint GetId() const;
  int width() const { return width_; }
  int height() const { return height_; }

private:
  GLuint texture_id_ = 0;
  int width_ = 0;
  int height_ = 0;
};