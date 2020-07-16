#include "gui/ui.h"
#include "common/logging.h"

void RenderVideoPlayer(const Texture &tex, bool &open_video, float scale,
                       int wnd_width, int wnd_height) {

  int img_width = tex.width();
  int img_height = tex.height();
  if (img_width > img_height) {
    img_height *= static_cast<float>(wnd_width) / img_width;
    img_width = wnd_width;
  } else {
    img_width *= static_cast<float>(wnd_height) / img_height;
    img_height = wnd_height;
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

void Frustum(float left, float right, float bottom, float top, float znear,
             float zfar, float *m16) {
  float temp, temp2, temp3, temp4;
  temp = 2.0f * znear;
  temp2 = right - left;
  temp3 = top - bottom;
  temp4 = zfar - znear;
  m16[0] = temp / temp2;
  m16[1] = 0.0;
  m16[2] = 0.0;
  m16[3] = 0.0;
  m16[4] = 0.0;
  m16[5] = temp / temp3;
  m16[6] = 0.0;
  m16[7] = 0.0;
  m16[8] = (right + left) / temp2;
  m16[9] = (top + bottom) / temp3;
  m16[10] = (-zfar - znear) / temp4;
  m16[11] = -1.0f;
  m16[12] = 0.0;
  m16[13] = 0.0;
  m16[14] = (-temp * zfar) / temp4;
  m16[15] = 0.0;
}

void Perspective(float fovyInDegrees, float aspectRatio, float znear,
                 float zfar, float *m16) {
  float ymax, xmax;
  ymax = znear * tanf(fovyInDegrees * 3.141592f / 180.0f);
  xmax = ymax * aspectRatio;
  Frustum(-xmax, xmax, -ymax, ymax, znear, zfar, m16);
}
void Cross(const float *a, const float *b, float *r) {
  r[0] = a[1] * b[2] - a[2] * b[1];
  r[1] = a[2] * b[0] - a[0] * b[2];
  r[2] = a[0] * b[1] - a[1] * b[0];
}

float Dot(const float *a, const float *b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void Normalize(const float *a, float *r) {
  float il = 1.f / (sqrtf(Dot(a, a)) + FLT_EPSILON);
  r[0] = a[0] * il;
  r[1] = a[1] * il;
  r[2] = a[2] * il;
}

void LookAt(const float *eye, const float *at, const float *up, float *m16) {
  float X[3], Y[3], Z[3], tmp[3];

  tmp[0] = eye[0] - at[0];
  tmp[1] = eye[1] - at[1];
  tmp[2] = eye[2] - at[2];
  // Z.normalize(eye - at);
  Normalize(tmp, Z);
  Normalize(up, Y);
  // Y.normalize(up);

  Cross(Y, Z, tmp);
  // tmp.cross(Y, Z);
  Normalize(tmp, X);
  // X.normalize(tmp);

  Cross(Z, X, tmp);
  // tmp.cross(Z, X);
  Normalize(tmp, Y);
  // Y.normalize(tmp);

  m16[0] = X[0];
  m16[1] = Y[0];
  m16[2] = Z[0];
  m16[3] = 0.0f;
  m16[4] = X[1];
  m16[5] = Y[1];
  m16[6] = Z[1];
  m16[7] = 0.0f;
  m16[8] = X[2];
  m16[9] = Y[2];
  m16[10] = Z[2];
  m16[11] = 0.0f;
  m16[12] = -Dot(X, eye);
  m16[13] = -Dot(Y, eye);
  m16[14] = -Dot(Z, eye);
  m16[15] = 1.0f;
}

void EditTransform(const float *cameraView, float *cameraProjection,
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

void RenderScene() {
  static float objectMatrix[4][16] = {{1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                       0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f},

                                      {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                       0.f, 0.f, 1.f, 0.f, 2.f, 0.f, 0.f, 1.f},

                                      {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                       0.f, 0.f, 1.f, 0.f, 2.f, 0.f, 2.f, 1.f},

                                      {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                       0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 2.f, 1.f}};

  static const float identityMatrix[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f,
                                           0.f, 0.f, 0.f, 0.f, 1.f, 0.f,
                                           0.f, 0.f, 0.f, 1.f};

  static float cameraView[16] = {1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
                                 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f};

  static float cameraProjection[16];
  static float fov = 27.f;
  static float viewWidth = 10.f; // for orthographic
  static float camYAngle = 165.f / 180.f * 3.14159f;
  static float camXAngle = 32.f / 180.f * 3.14159f;
  static float camDistance = 8.f;
  static int gizmoCount = 1;
  static int lastUsing = 0;
  static bool firstFrame = true;
  static bool isPerspective = true;
  ImGuiIO &io = ImGui::GetIO();

  Perspective(fov, io.DisplaySize.x / io.DisplaySize.y, 0.1f, 100.f,
              cameraProjection);
  ImGuizmo::SetOrthographic(false);
  ImGuizmo::BeginFrame();
  // ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);
  // ImGuizmo::DrawCubes(cameraView, cameraProjection, &objectMatrix[0][0], 1);

  ImGui::SetNextWindowPos(ImVec2(1024, 100));
  ImGui::SetNextWindowSize(ImVec2(256, 256));

  // create a window and insert the inspector
  ImGui::SetNextWindowPos(ImVec2(10, 10));
  ImGui::SetNextWindowSize(ImVec2(320, 340));
  ImGui::Begin("Editor");
  ImGui::Text("Camera");
  bool viewDirty = false;
  if (ImGui::RadioButton("Perspective", isPerspective))
    isPerspective = true;
  ImGui::SameLine();
  if (ImGui::RadioButton("Orthographic", !isPerspective))
    isPerspective = false;
  if (isPerspective) {
    ImGui::SliderFloat("Fov", &fov, 20.f, 110.f);
  } else {
    ImGui::SliderFloat("Ortho width", &viewWidth, 1, 20);
  }
  viewDirty |= ImGui::SliderFloat("Distance", &camDistance, 1.f, 10.f);
  ImGui::SliderInt("Gizmo count", &gizmoCount, 1, 4);

  if (viewDirty || firstFrame) {
    float eye[] = {cosf(camYAngle) * cosf(camXAngle) * camDistance,
                   sinf(camXAngle) * camDistance,
                   sinf(camYAngle) * cosf(camXAngle) * camDistance};
    float at[] = {0.f, 0.f, 0.f};
    float up[] = {0.f, 1.f, 0.f};
    LookAt(eye, at, up, cameraView);
    firstFrame = false;
  }

  ImGui::Text("X: %f Y: %f", io.MousePos.x, io.MousePos.y);
  ImGuizmo::DrawGrid(cameraView, cameraProjection, identityMatrix, 100.f);
  ImGuizmo::DrawCubes(cameraView, cameraProjection, &objectMatrix[0][0],
                      gizmoCount);
  ImGui::Separator();
  for (int matId = 0; matId < gizmoCount; matId++) {
    ImGuizmo::SetID(matId);

    EditTransform(cameraView, cameraProjection, objectMatrix[matId],
                  lastUsing == matId);
    if (ImGuizmo::IsUsing()) {
      lastUsing = matId;
    }
  }

  ImGui::End();

  ImGuizmo::ViewManipulate(cameraView, camDistance,
                           ImVec2(io.DisplaySize.x - 128, 0), ImVec2(128, 128),
                           0x10101010);
  // LOG(INFO) << "Print the camera view: ";
  // for (int i = 0; i < 16; i += 4) {
  //   std::cout << cameraView[i + 0] << " " << cameraView[i + 1] << " " <<
  //   cameraView[i + 2] << " " << cameraView[i + 3] << std::endl;
  // }
}