#include "render/gles_program.h"

#include <GLES3/gl3.h>

#include <stdexcept>
#include <string>

namespace ai3
{
namespace
{
std::uint32_t compile_shader(GLenum type, const char* source, std::string_view label)
{
    const std::uint32_t shader = glCreateShader(type);
    if (shader == 0)
        throw std::runtime_error(std::string(label) + " shader creation failed");
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE)
        return shader;
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(static_cast<std::size_t>(length > 0 ? length : 1), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error(std::string(label) + " shader compilation failed: " + log);
}
} // namespace

GlesProgram::GlesProgram(std::string_view label, const char* vertex_source,
                         const char* fragment_source)
{
    const std::uint32_t vertex = compile_shader(GL_VERTEX_SHADER, vertex_source, label);
    std::uint32_t fragment = 0;
    try
    {
        fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_source, label);
        id_ = glCreateProgram();
        if (id_ == 0)
            throw std::runtime_error(std::string(label) + " program creation failed");
        glAttachShader(id_, vertex);
        glAttachShader(id_, fragment);
        glLinkProgram(id_);
        GLint linked = GL_FALSE;
        glGetProgramiv(id_, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE)
        {
            GLint length = 0;
            glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<std::size_t>(length > 0 ? length : 1), '\0');
            glGetProgramInfoLog(id_, length, nullptr, log.data());
            throw std::runtime_error(std::string(label) + " program link failed: " + log);
        }
    }
    catch (...)
    {
        if (id_ != 0)
            glDeleteProgram(id_);
        id_ = 0;
        if (fragment != 0)
            glDeleteShader(fragment);
        glDeleteShader(vertex);
        throw;
    }
    glDeleteShader(fragment);
    glDeleteShader(vertex);
}

GlesProgram::~GlesProgram()
{
    if (id_ != 0)
        glDeleteProgram(id_);
}

GlesProgram::GlesProgram(GlesProgram&& other) noexcept : id_(other.id_) { other.id_ = 0; }

GlesProgram& GlesProgram::operator=(GlesProgram&& other) noexcept
{
    if (this == &other)
        return *this;
    if (id_ != 0)
        glDeleteProgram(id_);
    id_ = other.id_;
    other.id_ = 0;
    return *this;
}

int GlesProgram::required_uniform(const char* name) const
{
    const int location = glGetUniformLocation(id_, name);
    if (location < 0)
        throw std::runtime_error(std::string("Required GLES uniform is unavailable: ") + name);
    return location;
}
} // namespace ai3
