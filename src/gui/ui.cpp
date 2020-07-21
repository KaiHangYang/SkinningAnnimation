#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/common.hpp>
#include <memory>

#include "common/logging.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "gui/ui.h"

static void glfw_error_callback(int error, const char *desc) {
  LOG(WARNING) << "GLFW Error: " << error << ", " << desc;
}

bool App::Init(int wnd_width, int wnd_height, const std::string &title) {
  glfwSetErrorCallback(glfw_error_callback);
  CHECK(glfwInit()) << "GLFW Error: Failed to initiali  ze GLFW!";

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
  InitAvatar();

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

void App::InitAvatar() {
  tinygltf::TinyGLTF gltf_ctx;
  std::string err;
  std::string warn;
  bool ret = false;
  ret = gltf_ctx.LoadASCIIFromFile(&avatar_model_, &err, &warn, "Fox.gltf");

  if (!err.empty()) {
    LOG(ERROR) << err;
  }
  if (!warn.empty()) {
    LOG(WARNING) << warn;
  }
  if (!ret) {
    LOG(FATAL) << "Load gltf failed!";
  }

  auto gltfComponentByteSize = [](int type) -> size_t {
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

  glGenVertexArrays(1, &avatar_render_params_.vao);
  glBindVertexArray(avatar_render_params_.vao);
  avatar_render_params_.vbos.clear();

  avatar_render_params_.shader.InitFromFile("../shader/avatar_vs.glsl",
                                            "../shader/avatar_fs.glsl");

  for (size_t bv_idx = 0; bv_idx < avatar_model_.bufferViews.size(); ++bv_idx) {
    if (bv_idx != 0) continue;

    const auto &buffer_view = avatar_model_.bufferViews[bv_idx];
    //if (buffer_view.target == 0) {
    //  LOG(WARNING) << "buffer_view.target is 0.";
    //  continue;
    //}
    int sparce_accessor = -1;
    for (size_t a_idx = 0; a_idx < avatar_model_.accessors.size(); ++a_idx) {
      const auto &accessor = avatar_model_.accessors[a_idx];
      if (accessor.bufferView == bv_idx) {
        LOG(INFO) << "buffer_view is used by accessor " << a_idx;
        if (accessor.sparse.isSparse) {
          LOG(INFO)
              << "WARN: this bufferView has at least one sparse accessor to "
                 "it. We are going to load the data as patched by this "
                 "sparse accessor, not the original data";
          sparce_accessor = a_idx;
          break;
        }
      }
    }
    // Only bind POSITION buffer
    const auto &buffer = avatar_model_.buffers[buffer_view.buffer];
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    avatar_render_params_.vbos[bv_idx] = vbo;

    LOG(INFO) << "Buffer size: " << buffer.data.size()
              << ", buffer offset: " << buffer_view.byteOffset;
    if (sparce_accessor < 0) {
      // Copy raw data.
      glBufferData(buffer_view.target, buffer_view.byteLength,
                   &buffer.data.at(0) + buffer_view.byteOffset, GL_STATIC_DRAW);
    } else {
      const auto &accessor = avatar_model_.accessors[sparce_accessor];
      std::vector<uint8_t> sparce_data(buffer_view.byteLength, 0);
      std::copy(buffer.data.data() + buffer_view.byteOffset,
                buffer.data.data() + buffer_view.byteOffset +
                    buffer_view.byteLength,
                sparce_data.data());
      const size_t size_of_object_in_buffer =
          gltfComponentByteSize(accessor.componentType);
      const size_t size_of_sparse_indices =
          gltfComponentByteSize(accessor.sparse.indices.componentType);
      const auto &indices_buffer_view =
          avatar_model_.bufferViews[accessor.sparse.indices.bufferView];
      const auto &indices_buffer =
          avatar_model_.buffers[indices_buffer_view.buffer];

      const auto &values_buffer_view =
          avatar_model_.bufferViews[accessor.sparse.values.bufferView];
      const auto &values_buffer =
          avatar_model_.buffers[values_buffer_view.buffer];

      for (size_t sparse_idx = 0; sparse_idx < accessor.sparse.count;
           ++sparse_idx) {
        int index = 0;
        switch (accessor.sparse.indices.componentType) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
          index =
              (int)*(unsigned char *)(indices_buffer.data.data() +
                                      indices_buffer_view.byteOffset +
                                      accessor.sparse.indices.byteOffset +
                                      (sparse_idx * size_of_sparse_indices));
          break;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
          index =
              (int)*(unsigned short *)(indices_buffer.data.data() +
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
        std::cout << "updating sparse data at index  : " << index << std::endl;

        const uint8_t *read_from =
            values_buffer.data.data() +
            (values_buffer_view.byteOffset +
             accessor.sparse.values.byteOffset) +
            (sparse_idx * (size_of_object_in_buffer * accessor.type));

        uint8_t *write_to = sparce_data.data() +
                            index * (size_of_object_in_buffer * accessor.type);

        memcpy(write_to, read_from, size_of_object_in_buffer * accessor.type);
      }
      glBufferData(buffer_view.target, buffer_view.byteLength,
                   sparce_data.data(), GL_STATIC_DRAW);
    }
  }
  glBindVertexArray(0);
}

void App::RenderAvatarMesh(const tinygltf::Model &model,
                      const tinygltf::Mesh &mesh, const glm::mat4& parent_transform) {

  for (size_t p_idx = 0; p_idx < mesh.primitives.size(); ++p_idx) {
    const auto& primitive = mesh.primitives[p_idx];
    //if (primitive.indices < 0) continue;
    auto p_iter = primitive.attributes.begin();
    auto p_iter_end = primitive.attributes.end();

    int j_size = 0;
    for (; p_iter != p_iter_end; ++p_iter) {
      if (p_iter->first != "POSITION") continue;

      CHECK(p_iter->second >= 0) << "Primitive accessor must > 0";
      const auto& accessor = model.accessors[p_iter->second];
      glBindBuffer(GL_ARRAY_BUFFER, avatar_render_params_.vbos[accessor.bufferView]);
      int size = 1;
      if (accessor.type == TINYGLTF_TYPE_SCALAR) {
        size = 1;
      }
      else if (accessor.type == TINYGLTF_TYPE_VEC2) {
        size = 2;
      }
      else if (accessor.type == TINYGLTF_TYPE_VEC3) {
        size = 3;
      }
      else if (accessor.type == TINYGLTF_TYPE_VEC4) {
        size = 4;
      }
      else {
        CHECK(0) << "Don't support accessor.type: " << accessor.type;
      }
      if (p_iter->first == "POSITION") {
        int byte_stride = accessor.ByteStride(model.bufferViews[accessor.bufferView]);
        CHECK(byte_stride != -1) << "byte_strid equal -1";
        glVertexAttribPointer(0, size, accessor.componentType, accessor.normalized ? GL_TRUE : GL_FALSE, byte_stride, (GLvoid*)(accessor.byteOffset));
        glEnableVertexAttribArray(0);
      }

      j_size = accessor.count;
    }
    //const auto& index_accessor = model.accessors[primitive.indices];
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, avatar_render_params_.vbos[index_accessor.bufferView]);

    int mode = -1;
    if (primitive.mode == TINYGLTF_MODE_TRIANGLES) {
      mode = GL_TRIANGLES;
    }
    else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_STRIP) {
      mode = GL_TRIANGLE_STRIP;
    }
    else if (primitive.mode == TINYGLTF_MODE_TRIANGLE_FAN) {
      mode = GL_TRIANGLE_FAN;
    }
    else if (primitive.mode == TINYGLTF_MODE_POINTS) {
      mode = GL_POINTS;
    }
    else if (primitive.mode == TINYGLTF_MODE_LINE) {
      mode = GL_LINES;
    }
    else if (primitive.mode == TINYGLTF_MODE_LINE_LOOP) {
      mode = GL_LINE_LOOP;
    }
    else {
      CHECK(0) << "primitive.mode is invalid!";
    }

    //glDrawElements(mode, index_accessor.count, index_accessor.componentType, (GLvoid*)(index_accessor.byteOffset));
    glDrawArrays(mode, 0, j_size);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);
  }
}

void App::RenderAvatarNode(const tinygltf::Model &model,
                           const tinygltf::Node &node,
                           const glm::mat4& parent_transform) {
  glm::mat4 cur_transform(1.f);
  if (node.matrix.size() == 16) {
    std::copy(node.matrix.begin(), node.matrix.end(), &cur_transform[0][0]);
    cur_transform = cur_transform;
  }
  else {
    glm::vec3 scale_vec(1.0, 1.0, 1.0);
    glm::quat rot_vec(1.0, 0.0, 0.0, 0.0);
    glm::vec3 trans_vec(0.0, 0.0, 0.0);
    if (!node.scale.empty()) {
      scale_vec = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
    }
    if (!node.rotation.empty()) {
      rot_vec = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
    }
    if (!node.translation.empty()) {
      trans_vec = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
    }
    glm::mat4 cur_scale = glm::scale(glm::mat4(1.f), scale_vec);
    glm::mat4 cur_rot = glm::toMat4(rot_vec);
    glm::mat4 cur_trans = glm::translate(glm::mat4(1.f), trans_vec);
    cur_transform = cur_trans * cur_rot * cur_scale;
  }
  cur_transform = parent_transform * cur_transform;

  if (node.mesh > -1) {
    RenderAvatarMesh(model, model.meshes[node.mesh], parent_transform);
  }

  for (size_t c_idx = 0; c_idx < node.children.size(); ++c_idx) {
    RenderAvatarNode(model, model.nodes[node.children[c_idx]], cur_transform);
  }
}

void App::RenderAvatar(const glm::mat4& view_matrix, const glm::mat4& proj_matrix, const glm::mat4& model_matrix) {
  glBindVertexArray(avatar_render_params_.vao);
  avatar_render_params_.shader.Use();
  avatar_render_params_.shader.Set("model_matrix", model_matrix);
  avatar_render_params_.shader.Set("view_matrix", view_matrix);
  avatar_render_params_.shader.Set("proj_matrix", proj_matrix);
  GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
  GLboolean last_enable_multisample = glIsEnabled(GL_MULTISAMPLE);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);

  int scene_to_display =
      avatar_model_.defaultScene > -1 ? avatar_model_.defaultScene : 0;
  const tinygltf::Scene &scene = avatar_model_.scenes[scene_to_display];
  for (size_t n_idx = 0; n_idx < scene.nodes.size(); ++n_idx) {
    RenderAvatarNode(avatar_model_, avatar_model_.nodes[scene.nodes[n_idx]], glm::mat4(1.f));
  }
  glBindVertexArray(0);
  if (!last_enable_depth_test)
    glDisable(GL_DEPTH_TEST);
  if (!last_enable_multisample)
    glDisable(GL_MULTISAMPLE);
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
                         io.DisplaySize.x / io.DisplaySize.y, 0.001f, 10000.f);
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
      ImGui::SliderFloat("Distance", &camera_distance_, 3.f, 200.f);
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
  RenderAvatar(view_matrix_, proj_matrix_, model_matrix_);

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
