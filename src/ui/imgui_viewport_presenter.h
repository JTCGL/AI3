#pragma once

#include "render/viewport_renderer.h"

struct ImVec2;

namespace ai3
{
class ImGuiViewportPresenter
{
    public:
    static void present(ViewportOutput output, const ImVec2& size);
};
} // namespace ai3
