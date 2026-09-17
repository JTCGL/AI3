#pragma once

#include "core/workspace.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace ai3
{
class OrbitCamera
{
    public:
    explicit OrbitCamera(EditorViewState& state) : state_(state) {}
    glm::mat4 view_matrix() const;
    glm::mat4 projection_matrix(float aspect_ratio) const;
    glm::vec3 position() const;
    bool orbit(float yaw_delta_degrees, float pitch_delta_degrees);
    bool pan(glm::vec2 pointer_delta, float logical_viewport_height);
    bool zoom(float wheel_delta);
    void reset();

    float yaw_degrees() const { return state_.yaw_degrees; }
    float pitch_degrees() const { return state_.pitch_degrees; }
    float distance() const { return state_.distance; }
    const glm::vec3& target() const { return state_.target; }

    private:
    static constexpr float vertical_fov_degrees_ = 50.0F;
    EditorViewState& state_;
};
} // namespace ai3
