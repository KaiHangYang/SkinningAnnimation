#pragma once

#include <imgui.h>

#include "graphic/texture.h"

void RenderVideoPlayer(const Texture &tex, bool &open_video, float scale = 1.0,
                       int wnd_width = 256, int wnd_height = 148);