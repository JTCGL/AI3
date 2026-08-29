#pragma once

namespace ai3
{
struct RenderTargetSize
{
    int width = 0;
    int height = 0;
};

RenderTargetSize render_target_size(float logical_width, float logical_height,
                                    float framebuffer_scale_x, float framebuffer_scale_y);
bool requires_render_target_resize(RenderTargetSize current, RenderTargetSize requested);
} // namespace ai3
