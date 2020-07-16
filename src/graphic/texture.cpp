#include "graphic/texture.h"
#include "common/logging.h"

Texture::Texture() {
  glGenTextures(1, &texture_id_);
  glBindTexture(GL_TEXTURE_2D, texture_id_);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::~Texture() {
  // Release the texture.
  glDeleteTextures(1, &texture_id_);
}

GLuint Texture::GetId() const {
  if (width_ > 0 && height_ > 0)
    return texture_id_;
  else
    return 0;
}

bool Texture::LoadImage(const cv::Mat &img) {
  if (img.cols <= 0 && img.rows <= 0) {
    LOG(WARNING) << "Texture: input image is invalid!";
    return false;
  }

  width_ = img.cols;
  height_ = img.rows;

  GLenum external_formate = GL_BGRA;

  if (img.channels() == 1)
    external_formate = GL_RED;
  else if (img.channels() == 2)
    external_formate = GL_RG;
  else if (img.channels() == 3)
    external_formate = GL_BGR;
  else if (img.channels() == 4)
    external_formate = GL_BGRA;

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.cols, img.rows, 0, external_formate,
               GL_UNSIGNED_BYTE, img.data);

  return true;
}
