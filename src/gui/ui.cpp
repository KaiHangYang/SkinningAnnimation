#include <glm/gtc/matrix_transform.hpp>

#include "common/logging.h"
#include "gui/ui.h"

static void glfw_error_callback(int error, const char *desc) {
  LOG(WARNING) << "GLFW Error: " << error << ", " << desc;
}

bool App::Init(int wnd_width, int wnd_height, const std::string &title) {
  glfwSetErrorCallback(glfw_error_callback);
  CHECK(glfwInit()) << "GLFW Error: Failed to initialize GLFW!";

  // Use OpenGL 3.3
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_SAMPLES, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // 3.2+ only
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // 3.0+ only
  glfwWindowHint(GLFW_RESIZABLE, GL_TRUE); // fix window size
  const char *glsl_version = "#version 330";

  window_ = glfwCreateWindow(wnd_width, wnd_height, "Skinning Animation",
                             nullptr, nullptr);

  if (!window_)
    return inited_;

  glfwMakeContextCurrent(window_);
  // set v-sync, make the gpu frequency the same as
  // the screen refresh frequency
  glfwSwapInterval(1);

  bool success = gl3wInit() == 0;
  if (!success) {
    LOG(WARNING) << "GL3W: Failed to initialize OpenGL loader!";
    return inited_;
  }
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
  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init(glsl_version);

  wnd_width_ = wnd_width;
  wnd_height_ = wnd_height;

  show_video_ = true;
  video_scale_ = 1.0;

  // Init scene params.
  proj_matrix_ = glm::perspective(
      fov_ / 180.f * MY_PI, io.DisplaySize.x / io.DisplaySize.y, 0.001f, 100.f);
  {
    // Set view_matrix_
    glm::vec3 eye{
        cosf(camera_angle_.y) * cosf(camera_angle_.x) * camera_distance_,
        sinf(camera_angle_.x) * camera_distance_,
        sinf(camera_angle_.y) * cosf(camera_angle_.x) * camera_distance_};
    glm::vec3 at{0.f, 0.f, 0.f};
    glm::vec3 up{0.f, 1.f, 0.f};
    view_matrix_ = glm::lookAt(eye, at, up);
  }

  InitPlane();

  inited_ = true;
  return inited_;
}

void App::InitPlane() {
  plane_render_params_.shader.InitFromFile("../shader/ground_vs.glsl",
                             "../shader/ground_fs.glsl");
  Geometry::Grid plane_grid;
  plane_grid.Init(100.0, 5.0);

  std::vector<float> plane_vertex_data;
  std::vector<float> plane_normal_data;
  std::vector<float> plane_color_data;
  std::vector<float> plane_data;

  plane_grid.GetData(plane_vertex_data, plane_normal_data, plane_color_data);
  plane_render_params_.vertex_num = plane_vertex_data.size() / 3;

  int vertex_offset = plane_data.size();
  plane_data.insert(plane_data.end(), plane_vertex_data.begin(),
                    plane_vertex_data.end());
  int normal_offset = plane_data.size();
  plane_data.insert(plane_data.end(), plane_normal_data.begin(),
                    plane_normal_data.end());
  int color_offset = plane_data.size();
  plane_data.insert(plane_data.end(), plane_color_data.begin(),
                    plane_color_data.end());

  glGenVertexArrays(1, &plane_render_params_.vao);
  glBindVertexArray(plane_render_params_.vao);

  glGenBuffers(1, &plane_render_params_.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, plane_render_params_.vbo);

  glBufferData(GL_ARRAY_BUFFER, plane_data.size() * sizeof(float),
               plane_data.data(), GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(vertex_offset * sizeof(float)));
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(normal_offset * sizeof(float)));
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0,
                        (GLvoid *)(color_offset * sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

int App::MainLoop() {
  if (!inited_) {
    LOG(WARNING) << "App hasn't been inited!";
    return -1;
  }

  // Video For Debug.
  cv::Mat img;
  cv::VideoCapture cap("E:/Video_To_Test/0055.mp4");
  Texture img_tex;

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int display_w, display_h;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color_.x, clear_color_.y, clear_color_.z,
                 clear_color_.w);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render Some thing include GUI.
    // Video visualizer.
    if (show_video_) {
      img_tex.LoadImage(img);
      RenderVideoPlayer(img_tex, show_video_, video_scale_);
    }
    RenderScene();

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
    if (cap.read(cur_img))
      img = cur_img;

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window_);
  glfwTerminate();

  return 0;
}

void App::RenderVideoPlayer(const Texture &tex, bool &open_video, float scale,
                            int widget_width, int widget_height) {

  int img_width = tex.width();
  int img_height = tex.height();
  if (img_width > img_height) {
    img_height *= static_cast<float>(widget_width) / img_width;
    img_width = widget_width;
  } else {
    img_width *= static_cast<float>(widget_height) / img_height;
    img_height = widget_height;
  }
  img_width *= scale;
  img_height *= scale;

  ImGui::SetNextWindowSize(ImVec2(img_width, img_height), ImGuiCond_Always);
  ImGuiWindowFlags window_flags = 0;
  window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
  window_flags |= ImGuiWindowFlags_NoNav;
  window_flags |= ImGuiWindowFlags_NoDecoration;
  // window_flags |= ImGuiWindowFlags_NoInputs;
  window_flags |= ImGuiWindowFlags_NoBackground;

  if (ImGui::Begin("Video", &open_video, window_flags)) {
    ImGui::Image(reinterpret_cast<ImTextureID>(tex.GetId()),
                 ImVec2(img_width, img_height));
  }
  ImGui::End();
}

void App::RenderPlane(const glm::mat4 &view_matrix,
                      const glm::mat4 &proj_matrix,
                      const glm::mat4 &model_matrix) {

  plane_render_params_.shader.Use();
  plane_render_params_.shader.Set("model_matrix", model_matrix);
  plane_render_params_.shader.Set("view_matrix", view_matrix);
  plane_render_params_.shader.Set("proj_matrix", proj_matrix);
  glBindVertexArray(plane_render_params_.vao);
  GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
  GLboolean last_enable_multisample = glIsEnabled(GL_MULTISAMPLE);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);

  glDrawArrays(GL_TRIANGLES, 0, plane_render_params_.vertex_num);

  glBindVertexArray(0);

  if (!last_enable_depth_test)
    glDisable(GL_DEPTH_TEST);
  if (!last_enable_multisample)
    glDisable(GL_MULTISAMPLE);
}

void App::RenderScene() {
  ImGuiIO &io = ImGui::GetIO();
  if (is_perspective_) {
    proj_matrix_ =
        glm::perspective(fov_ / 180.f * MY_PI,
                         io.DisplaySize.x / io.DisplaySize.y, 0.001f, 100.f);
  } else {
    float view_height = view_width_ * io.DisplaySize.y / io.DisplaySize.x;
    proj_matrix_ = glm::ortho(-view_width_, view_width_, -view_height,
                              view_height, -1000.0f, 1000.0f);
  }

  ImGuizmo::SetOrthographic(!is_perspective_);
  ImGuizmo::BeginFrame();

  // create a window and insert the inspector
  ImGui::SetNextWindowPos(ImVec2(10, 10));
  ImGui::SetNextWindowSize(ImVec2(320, 340));
  ImGui::Begin("Editor");
  ImGui::Checkbox("show video.", &show_video_);
  ImGui::SliderFloat("video scale.", &video_scale_, 0.5f, 3.0f, "%.2f");
  ImGui::Separator();

  ImGui::Text("Camera");
  if (ImGui::RadioButton("Perspective", is_perspective_))
    is_perspective_ = true;
  ImGui::SameLine();
  if (ImGui::RadioButton("Orthographic", !is_perspective_))
    is_perspective_ = false;

  if (is_perspective_) {
    ImGui::SliderFloat("Fov", &fov_, 20.f, 110.f);
  } else {
    ImGui::SliderFloat("Ortho width", &view_width_, 1, 20);
  }
  bool view_dirty =
      ImGui::SliderFloat("Distance", &camera_distance_, 3.f, 30.f);
  if (view_dirty) {
    // Set view_matrix_
    glm::vec3 eye{
        cosf(camera_angle_.y) * cosf(camera_angle_.x) * camera_distance_,
        sinf(camera_angle_.x) * camera_distance_,
        sinf(camera_angle_.y) * cosf(camera_angle_.x) * camera_distance_};
    glm::vec3 at{0.f, 0.f, 0.f};
    glm::vec3 up{0.f, 1.f, 0.f};
    view_matrix_ = glm::lookAt(eye, at, up);
  }

  ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
  glm::mat4 grid_model_matrix(1.f);

  // ImGuizmo::DrawCubes(&view_matrix_[0][0], &proj_matrix_[0][0],
  //                     &model_matrix_[0][0], 1);
  RenderPlane(view_matrix_, proj_matrix_, model_matrix_);

  ImGui::Separator();
  ImGuizmo::SetID(0);
  EditTransform(&view_matrix_[0][0], &proj_matrix_[0][0], &model_matrix_[0][0],
                true);
  ImGui::End();

  ImGuizmo::ViewManipulate(&view_matrix_[0][0], camera_distance_,
                           ImVec2(io.DisplaySize.x - 128, 0), ImVec2(128, 128),
                           0x10101010);
}

void App::EditTransform(const float *cameraView, float *cameraProjection,
                        float *matrix, bool editTransformDecomposition) {
  static ImGuizmo::OPERATION mCurrentGizmoOperation(ImGuizmo::TRANSLATE);
  static ImGuizmo::MODE mCurrentGizmoMode(ImGuizmo::LOCAL);
  static bool useSnap = false;
  static float snap[3] = {1.f, 1.f, 1.f};
  static float bounds[] = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
  static float boundsSnap[] = {0.1f, 0.1f, 0.1f};
  static bool boundSizing = false;
  static bool boundSizingSnap = false;

  if (editTransformDecomposition) {
    if (ImGui::IsKeyPressed(90))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    if (ImGui::IsKeyPressed(69))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    if (ImGui::IsKeyPressed(82)) // r Key
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    if (ImGui::RadioButton("Translate",
                           mCurrentGizmoOperation == ImGuizmo::TRANSLATE))
      mCurrentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate",
                           mCurrentGizmoOperation == ImGuizmo::ROTATE))
      mCurrentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", mCurrentGizmoOperation == ImGuizmo::SCALE))
      mCurrentGizmoOperation = ImGuizmo::SCALE;
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents(matrix, matrixTranslation,
                                          matrixRotation, matrixScale);
    ImGui::InputFloat3("Tr", matrixTranslation, 3);
    ImGui::InputFloat3("Rt", matrixRotation, 3);
    ImGui::InputFloat3("Sc", matrixScale, 3);
    ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                            matrixScale, matrix);

    if (mCurrentGizmoOperation != ImGuizmo::SCALE) {
      if (ImGui::RadioButton("Local", mCurrentGizmoMode == ImGuizmo::LOCAL))
        mCurrentGizmoMode = ImGuizmo::LOCAL;
      ImGui::SameLine();
      if (ImGui::RadioButton("World", mCurrentGizmoMode == ImGuizmo::WORLD))
        mCurrentGizmoMode = ImGuizmo::WORLD;
    }
    if (ImGui::IsKeyPressed(83))
      useSnap = !useSnap;
    ImGui::Checkbox("", &useSnap);
    ImGui::SameLine();

    switch (mCurrentGizmoOperation) {
    case ImGuizmo::TRANSLATE:
      ImGui::InputFloat3("Snap", &snap[0]);
      break;
    case ImGuizmo::ROTATE:
      ImGui::InputFloat("Angle Snap", &snap[0]);
      break;
    case ImGuizmo::SCALE:
      ImGui::InputFloat("Scale Snap", &snap[0]);
      break;
    }
    ImGui::Checkbox("Bound Sizing", &boundSizing);
    if (boundSizing) {
      ImGui::PushID(3);
      ImGui::Checkbox("", &boundSizingSnap);
      ImGui::SameLine();
      ImGui::InputFloat3("Snap", boundsSnap);
      ImGui::PopID();
    }
  }
  ImGuiIO &io = ImGui::GetIO();
  ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
  ImGuizmo::Manipulate(cameraView, cameraProjection, mCurrentGizmoOperation,
                       mCurrentGizmoMode, matrix, NULL,
                       useSnap ? &snap[0] : NULL, boundSizing ? bounds : NULL,
                       boundSizingSnap ? boundsSnap : NULL);
}
