#include "gui/ui.h"
#include "common/logging.h"

void RenderVideoPlayer(const Texture &tex, bool &open_video, float scale, int wnd_width,
                       int wnd_height) {

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