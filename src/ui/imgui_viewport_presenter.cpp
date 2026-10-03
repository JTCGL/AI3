#include "ui/imgui_viewport_presenter.h"

#include "imgui.h"

namespace ai3
{
class GlesViewportOutputAccess
{
    public:
    static std::uint32_t texture(ViewportOutput output) { return output.texture_; }
};

void ImGuiViewportPresenter::present(ViewportOutput output, const ImVec2& size)
{
    ImGui::Image(static_cast<ImTextureID>(GlesViewportOutputAccess::texture(output)), size,
                 {0.0F, 1.0F}, {1.0F, 0.0F});
}
} // namespace ai3
