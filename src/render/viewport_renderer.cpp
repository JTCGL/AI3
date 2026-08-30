#include "render/viewport_renderer.h"

#include "scene/scene_math.h"
#include "scene/sphere_mesh.h"

#include <GLES3/gl3.h>

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace ai3
{
namespace
{
constexpr const char* vertex_shader_source = R"(#version 300 es
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 v_normal;
void main()
{
    v_normal = mat3(transpose(inverse(u_model))) * a_normal;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

constexpr const char* fragment_shader_source = R"(#version 300 es
precision mediump float;
in vec3 v_normal;
out vec4 out_color;
void main()
{
    vec3 normal = normalize(v_normal);
    vec3 light = normalize(vec3(0.5, 0.8, 0.7));
    float diffuse = max(dot(normal, light), 0.0);
    vec3 base = vec3(0.22, 0.58, 0.92);
    out_color = vec4(base * (0.25 + diffuse * 0.75), 1.0);
}
)";

std::uint32_t compile_shader(GLenum type, const char* source)
{
    const std::uint32_t shader = glCreateShader(type);
    if (shader == 0)
        throw std::runtime_error("Viewport shader creation failed");
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
    throw std::runtime_error("Viewport shader compilation failed: " + log);
}

std::uint32_t create_program()
{
    const std::uint32_t vertex = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    std::uint32_t fragment = 0;
    std::uint32_t program = 0;
    try
    {
        fragment = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
        program = glCreateProgram();
        if (program == 0)
            throw std::runtime_error("Viewport shader program creation failed");
        glAttachShader(program, vertex);
        glAttachShader(program, fragment);
        glLinkProgram(program);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE)
        {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<std::size_t>(length > 0 ? length : 1), '\0');
            glGetProgramInfoLog(program, length, nullptr, log.data());
            throw std::runtime_error("Viewport shader link failed: " + log);
        }
    }
    catch (...)
    {
        if (program != 0)
            glDeleteProgram(program);
        if (fragment != 0)
            glDeleteShader(fragment);
        glDeleteShader(vertex);
        throw;
    }
    glDeleteShader(fragment);
    glDeleteShader(vertex);
    return program;
}

std::string gl_string(GLenum name)
{
    const GLubyte* value = glGetString(name);
    return value == nullptr ? "unknown" : reinterpret_cast<const char*>(value);
}

void require_no_gl_error(const char* operation)
{
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        throw std::runtime_error(std::string(operation) + " failed with OpenGL ES error " +
                                 std::to_string(error));
}
} // namespace

ViewportRenderer::ViewportRenderer()
{
    try
    {
        program_ = create_program();
        mvp_location_ = glGetUniformLocation(program_, "u_mvp");
        model_location_ = glGetUniformLocation(program_, "u_model");
        if (mvp_location_ < 0 || model_location_ < 0)
            throw std::runtime_error("Viewport shader uniforms are unavailable");

        gl_description_ =
            gl_string(GL_VERSION) + " | " + gl_string(GL_VENDOR) + " | " + gl_string(GL_RENDERER);
        require_no_gl_error("Viewport renderer initialization");
    }
    catch (...)
    {
        if (program_ != 0)
            glDeleteProgram(program_);
        throw;
    }
}

ViewportRenderer::~ViewportRenderer()
{
    clear_geometry_cache();
    destroy_render_target();
    if (program_ != 0)
        glDeleteProgram(program_);
}

void ViewportRenderer::destroy_geometry(SphereGeometry& geometry)
{
    if (geometry.index_buffer != 0)
        glDeleteBuffers(1, &geometry.index_buffer);
    if (geometry.vertex_buffer != 0)
        glDeleteBuffers(1, &geometry.vertex_buffer);
    if (geometry.vertex_array != 0)
        glDeleteVertexArrays(1, &geometry.vertex_array);
    geometry = {};
}

void ViewportRenderer::clear_geometry_cache()
{
    for (auto& entry : sphere_geometry_cache_)
        destroy_geometry(entry.second);
    sphere_geometry_cache_.clear();
}

void ViewportRenderer::synchronize_geometry_cache(const EditorState& scene)
{
    std::unordered_set<ObjectId> live_ids;
    for (const SceneObject& object : scene.objects())
        live_ids.insert(object.id);
    for (auto it = sphere_geometry_cache_.begin(); it != sphere_geometry_cache_.end();)
    {
        if (live_ids.count(it->first) != 0)
        {
            ++it;
            continue;
        }
        destroy_geometry(it->second);
        it = sphere_geometry_cache_.erase(it);
    }
}

ViewportRenderer::SphereGeometry& ViewportRenderer::sphere_geometry(const SceneObject& object)
{
    auto [it, inserted] = sphere_geometry_cache_.try_emplace(object.id);
    SphereGeometry& geometry = it->second;
    if (!inserted && geometry.radius_meters == object.sphere.radius_meters)
        return geometry;

    destroy_geometry(geometry);
    const SphereMesh mesh = make_sphere_mesh(object.sphere.radius_meters);
    geometry.radius_meters = object.sphere.radius_meters;
    geometry.index_count = static_cast<std::uint32_t>(mesh.indices.size());
    glGenVertexArrays(1, &geometry.vertex_array);
    glGenBuffers(1, &geometry.vertex_buffer);
    glGenBuffers(1, &geometry.index_buffer);
    if (geometry.vertex_array == 0 || geometry.vertex_buffer == 0 || geometry.index_buffer == 0)
    {
        destroy_geometry(geometry);
        throw std::runtime_error("Viewport sphere resource creation failed");
    }
    glBindVertexArray(geometry.vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, geometry.vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(MeshVertex)),
                 mesh.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry.index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(std::uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<const void*>(offsetof(MeshVertex, position)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<const void*>(offsetof(MeshVertex, normal)));
    glBindVertexArray(0);
    return geometry;
}

void ViewportRenderer::destroy_render_target()
{
    if (depth_renderbuffer_ != 0)
        glDeleteRenderbuffers(1, &depth_renderbuffer_);
    if (color_texture_ != 0)
        glDeleteTextures(1, &color_texture_);
    if (framebuffer_ != 0)
        glDeleteFramebuffers(1, &framebuffer_);
    depth_renderbuffer_ = 0;
    color_texture_ = 0;
    framebuffer_ = 0;
    size_ = {};
}

void ViewportRenderer::resize(RenderTargetSize size)
{
    destroy_render_target();
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glGenTextures(1, &color_texture_);
    glBindTexture(GL_TEXTURE_2D, color_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.width, size.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture_, 0);
    glGenRenderbuffers(1, &depth_renderbuffer_);
    if (framebuffer_ == 0 || color_texture_ == 0 || depth_renderbuffer_ == 0)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        destroy_render_target();
        throw std::runtime_error("Viewport framebuffer resource creation failed");
    }
    glBindRenderbuffer(GL_RENDERBUFFER, depth_renderbuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.width, size.height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                              depth_renderbuffer_);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        destroy_render_target();
        throw std::runtime_error("Viewport framebuffer is incomplete");
    }
    size_ = size;
    ++resize_count_;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    require_no_gl_error("Viewport framebuffer resize");
}

void ViewportRenderer::render(const EditorState& scene, const OrbitCamera& camera,
                              RenderTargetSize size)
{
    synchronize_geometry_cache(scene);
    if (requires_render_target_resize(size_, size))
        resize(size);
    if (size_.width == 0 || size_.height == 0)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, size_.width, size_.height);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClearColor(0.055F, 0.07F, 0.10F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(program_);
    const glm::mat4 view_projection = camera.projection_matrix(static_cast<float>(size_.width) /
                                                               static_cast<float>(size_.height)) *
                                      camera.view_matrix();
    for (const SceneObject* object : scene.visible_spheres())
    {
        const SphereGeometry& geometry = sphere_geometry(*object);
        glBindVertexArray(geometry.vertex_array);
        const glm::mat4 model = compose_transform(object->transform);
        const glm::mat4 mvp = view_projection * model;
        glUniformMatrix4fv(mvp_location_, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(model_location_, 1, GL_FALSE, glm::value_ptr(model));
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(geometry.index_count), GL_UNSIGNED_INT,
                       nullptr);
    }
    glBindVertexArray(0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    require_no_gl_error("Viewport scene render");
}
} // namespace ai3
