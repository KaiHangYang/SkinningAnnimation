#pragma once

#include <cstdio>
#include <imgui.h>
#include <iostream>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include "common/utility.h"
#include "graphic/geometry.h"
#include "graphic/shader.h"
#include "graphic/texture.h"
#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl3.h"
#include "gui/ui.h"


class App {
public:
  App() = default;
  ~App(){};
  bool Init(int wnd_width = 1280, int wnd_height = 720,
            const std::string &title = "Skinning Animation");
  int MainLoop();

private:
  void RenderVideoPlayer(const Texture &tex, bool &open_video,
                         float scale = 1.0, int widget_width = 256,
                         int widget_height = 148);
  void RenderScene();
  void EditTransform(const float *cameraView, float *cameraProjection,
                     float *matrix, bool editTransformDecomposition);

  // for plane
  struct PlaneRenderParams {
    Shader shader;
    GLuint vao = 0;
    GLuint vbo = 0;
    int vertex_num = 0;
  };
  PlaneRenderParams plane_render_params_;
  void InitPlane();
  void RenderPlane(const glm::mat4 &view_matrix, const glm::mat4 &proj_matrix,
                   const glm::mat4 &model_matrix);
  
  // for avatar

  struct CameraAngle {
    float x = 165.f / 180.f * MY_PI;
    float y = 32.f / 180.f * MY_PI;
  };

  bool inited_ = false;

  int wnd_width_ = -1;
  int wnd_height_ = -1;
  GLFWwindow *window_ = nullptr;

  // GUI variables.
  bool show_video_ = true;
  float video_scale_ = 1.0;
  ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Scene parameters.
  bool is_perspective_ = true;
  float fov_ = 27.f;
  float view_width_ = 10.f; // for orthographic

  // for camera
  CameraAngle camera_angle_;
  float camera_distance_ = 8.f;

  glm::mat4 model_matrix_ = glm::mat4(1.f);
  glm::mat4 view_matrix_ = glm::mat4(1.f);
  glm::mat4 proj_matrix_ = glm::mat4(1.f);
};
