#include "scene/render_target_size.h"

#include <algorithm>
#include <cmath>

namespace ai3
{
RenderTargetSize render_target_size(float logical_width, float logical_height,
                                    float framebuffer_scale_x, float framebuffer_scale_y)
{
    if (logical_width <= 0.0F || logical_height <= 0.0F || framebuffer_scale_x <= 0.0F ||
        framebuffer_scale_y <= 0.0F)
        return {};
    return {std::max(1, static_cast<int>(std::lround(logical_width * framebuffer_scale_x))),
            std::max(1, static_cast<int>(std::lround(logical_height * framebuffer_scale_y)))};
}

bool requires_render_target_resize(RenderTargetSize current, RenderTargetSize requested)
{
    return requested.width > 0 && requested.height > 0 &&
           (current.width != requested.width || current.height != requested.height);
}
} // namespace ai3
