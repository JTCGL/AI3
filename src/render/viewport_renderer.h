#pragma once

#include "editor/editor_state.h"
#include "scene/helper_geometry.h"
#include "scene/render_target_size.h"
#include "scene/resolved_view.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace ai3
{
struct DepthTestedHelperInputs
{
    const HelperGeometry* bounds = nullptr;
    const HelperGeometry* gizmo = nullptr;
    const ResolvedViewportView* gizmo_view = nullptr;
};

class ViewportRenderer
{
    public:
    ViewportRenderer();
    ~ViewportRenderer();
    ViewportRenderer(const ViewportRenderer&) = delete;
    ViewportRenderer& operator=(const ViewportRenderer&) = delete;

    void render(const EditorState& scene, const ResolvedViewportView& view, RenderTargetSize size,
                DepthTestedHelperInputs helpers = {});
    void synchronize_geometry_cache(const EditorState& scene);
    void clear_geometry_cache();
    std::uint32_t texture() const { return color_texture_; }
    RenderTargetSize size() const { return size_; }
    std::uint64_t resize_count() const { return resize_count_; }
    const std::string& gl_description() const { return gl_description_; }

    private:
    void resize(RenderTargetSize size);
    void destroy_render_target();
    void render_helpers(const HelperGeometry& helpers, const glm::mat4& view_projection);

    struct SphereGeometry
    {
        float radius_meters = 0.0F;
        std::uint32_t vertex_array = 0;
        std::uint32_t vertex_buffer = 0;
        std::uint32_t index_buffer = 0;
        std::uint32_t index_count = 0;
    };

    SphereGeometry& sphere_geometry(const SceneObject& object);
    static void destroy_geometry(SphereGeometry& geometry);

    struct UnlitProgram;
    struct LambertProgram;
    struct PhongProgram;
    struct HelperProgram;

    std::unique_ptr<UnlitProgram> unlit_program_;
    std::unique_ptr<LambertProgram> lambert_program_;
    std::unique_ptr<PhongProgram> phong_program_;
    std::unique_ptr<HelperProgram> helper_program_;
    std::uint32_t helper_vertex_array_ = 0;
    std::uint32_t helper_vertex_buffer_ = 0;
    std::uint32_t framebuffer_ = 0;
    std::uint32_t color_texture_ = 0;
    std::uint32_t depth_renderbuffer_ = 0;
    RenderTargetSize size_{};
    std::uint64_t resize_count_ = 0;
    std::string gl_description_;
    std::unordered_map<ObjectId, SphereGeometry> sphere_geometry_cache_;
};
} // namespace ai3
