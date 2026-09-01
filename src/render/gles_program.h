#pragma once

#include <cstdint>
#include <string_view>

namespace ai3
{
class GlesProgram
{
    public:
    GlesProgram(std::string_view label, const char* vertex_source, const char* fragment_source);
    ~GlesProgram();
    GlesProgram(const GlesProgram&) = delete;
    GlesProgram& operator=(const GlesProgram&) = delete;
    GlesProgram(GlesProgram&& other) noexcept;
    GlesProgram& operator=(GlesProgram&& other) noexcept;

    std::uint32_t id() const { return id_; }
    int required_uniform(const char* name) const;

    private:
    std::uint32_t id_ = 0;
};
} // namespace ai3
