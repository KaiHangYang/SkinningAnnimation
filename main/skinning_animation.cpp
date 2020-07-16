#include <cstdio>
#include <imgui.h>
#include <ImGuizmo.h>
#include <iostream>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>

#include "common/logging.h"
#include "graphic/texture.h"
#include "graphic/utility.h"
#include "gui/imgui_impl_glfw.h"
#include "gui/imgui_impl_opengl3.h"
#include "gui/scene.h"
#include "gui/ui.h"

static void glfw_error_callback(int error, const char *desc) {
  LOG(WARNING) << "GLFW Error: " << error << ", " << desc;
}

int main(int argc, char **argv) {
  cv::VideoCapture video_cap = cv::VideoCapture("E:/Video_To_Test/0055.mp4");
  cv::Mat img;
  video_cap.read(img);

  glfwSetErrorCallback(glfw_error_callback);

  CHECK(glfwInit()) << "GLFW Error: Failed to initialize GLFW!";

  // Use OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE); // fix window size
  const char *glsl_version = "#version 330";

  GLFWwindow *window = glfwCreateWindow(1280, 720,
                                        "Skinning Animation", nullptr, nullptr);

  if (!window)
    return 1;

  glfwMakeContextCurrent(window);
  // set v-sync, make the gpu frequency the same as
  // the screen refresh frequency
  glfwSwapInterval(1);

  bool success = gl3wInit() == 0;
  CHECK(success) << "GL3W: Failed to initialize OpenGL loader!";
  // OpenGL Inited!

  // Setup gui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
  // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
  // Enable Gamepad Controls

  ImGui::StyleColorsClassic();

  // Setup Platform/Renderer bindings
  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  Scene scene;
  Texture img_tex;

  // <<<<<<<<<<<<<<<<<<<<<< Controller variables <<<<<<<<<<<<<<<<<<<<<<<<<
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  bool show_video = true;
  float video_scale = 1.0;
  // <<<<<<<<<<<<<<<<<<<<<< Controller variables <<<<<<<<<<<<<<<<<<<<<<<<<
  // Main Loop
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Controller
    {
      ImGui::Begin("Controller", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
      ImGui::Checkbox("show video.", &show_video);
      ImGui::SliderFloat("video scale.", &video_scale, 0.5f, 3.0f, "%.2f");
      ImGui::Separator();
      ImGui::End();
    }

    // Video visualizer.
    if (show_video) {
      img_tex.LoadImage(img);
      RenderVideoPlayer(img_tex, show_video, video_scale);
    }

    RenderScene();

    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();

    // Frame buffer coord in opengl is
    // |
    // |_______
    // but in image coordinate is
    // ________
    // |
    // |
    // so I need to flip vertically

    // glReadBuffer(GL_BACK);
    // glReadPixels(0, 0, display_w, display_h, GL_BGR, GL_FLOAT,
    // &frame_img[0]); cv::Mat tmp_img(wnd_height, wnd_width, CV_32FC3,
    // &frame_img[0]); cv::flip(tmp_img, tmp_img, 0); cv::imshow("test",
    // tmp_img); cv::waitKey(1);

    cv::Mat cur_img;
    if (video_cap.read(cur_img))
      img = cur_img;
    // scene.Draw(img);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}