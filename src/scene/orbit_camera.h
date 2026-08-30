#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace ai3
{
class OrbitCamera
{
    public:
    glm::mat4 view_matrix() const;
    glm::mat4 projection_matrix(float aspect_ratio) const;
    glm::vec3 position() const;
    void orbit(float yaw_delta_degrees, float pitch_delta_degrees);
    void zoom(float wheel_delta);
    void reset();

    float yaw_degrees() const { return yaw_degrees_; }
    float pitch_degrees() const { return pitch_degrees_; }
    float distance() const { return distance_; }
    const glm::vec3& target() const { return target_; }

    private:
    glm::vec3 target_{};
    float yaw_degrees_ = 35.0F;
    float pitch_degrees_ = 20.0F;
    float distance_ = 6.0F;
};
} // namespace ai3
