#pragma once

#include "editor/editor_state.h"
#include "scene/orbit_camera.h"
#include "scene/render_target_size.h"

#include <cstdint>
#include <string>

namespace ai3
{
class ViewportRenderer
{
    public:
    ViewportRenderer();
    ~ViewportRenderer();
    ViewportRenderer(const ViewportRenderer&) = delete;
    ViewportRenderer& operator=(const ViewportRenderer&) = delete;

    void render(const EditorState& scene, const OrbitCamera& camera, RenderTargetSize size);
    std::uint32_t texture() const { return color_texture_; }
    RenderTargetSize size() const { return size_; }
    std::uint64_t resize_count() const { return resize_count_; }
    const std::string& gl_description() const { return gl_description_; }

    private:
    void resize(RenderTargetSize size);
    void destroy_render_target();

    std::uint32_t program_ = 0;
    std::uint32_t vertex_array_ = 0;
    std::uint32_t vertex_buffer_ = 0;
    std::uint32_t index_buffer_ = 0;
    std::uint32_t framebuffer_ = 0;
    std::uint32_t color_texture_ = 0;
    std::uint32_t depth_renderbuffer_ = 0;
    int mvp_location_ = -1;
    int model_location_ = -1;
    RenderTargetSize size_{};
    std::uint64_t resize_count_ = 0;
    std::string gl_description_;
};
} // namespace ai3
