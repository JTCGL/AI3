#include "render/viewport_renderer.h"

#include "render/gles_program.h"
#include "scene/box_mesh.h"
#include "scene/scene_math.h"
#include "scene/sphere_mesh.h"

#include <GLES3/gl3.h>

#include <glm/gtc/type_ptr.hpp>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

namespace ai3
{
namespace
{
constexpr const char* unlit_vertex_source = R"(#version 300 es
layout(location = 0) in vec3 a_position;
uniform mat4 u_mvp;
void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

constexpr const char* lit_vertex_source = R"(#version 300 es
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
uniform mat4 u_mvp;
uniform mat4 u_model;
out vec3 v_normal;
out vec3 v_world_position;
void main()
{
    v_normal = mat3(transpose(inverse(u_model))) * a_normal;
    v_world_position = vec3(u_model * vec4(a_position, 1.0));
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

// Visual-only NDC depth bias. It does not modify semantic or cached geometry.
constexpr float helper_depth_bias = 0.0001F;
void apply_depth_state(HelperDepthState state)
{
    if (state.depth_test_enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    glDepthMask(state.depth_write_enabled ? GL_TRUE : GL_FALSE);
}

constexpr const char* helper_vertex_source = R"(#version 300 es
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_color;
uniform mat4 u_view_projection;
uniform float u_depth_bias;
out vec3 v_color;
void main()
{
    gl_Position = u_view_projection * vec4(a_position, 1.0);
    gl_Position.z -= u_depth_bias * gl_Position.w;
    v_color = a_color;
}
)";
constexpr const char* helper_fragment_source = R"(#version 300 es
precision mediump float;
in vec3 v_color;
out vec4 out_color;
void main() { out_color = vec4(v_color, 1.0); }
)";

constexpr const char* linear_to_srgb_source = R"(
vec3 linear_to_srgb(vec3 color)
{
    bvec3 cutoff = lessThanEqual(color, vec3(0.0031308));
    vec3 low = color * 12.92;
    vec3 high = 1.055 * pow(max(color, vec3(0.0)), vec3(1.0 / 2.4)) - 0.055;
    return mix(high, low, cutoff);
}
)";

std::string fragment_source(std::string_view declarations, std::string_view body)
{
    return std::string{"#version 300 es\nprecision mediump float;\n"} + std::string{declarations} +
           linear_to_srgb_source + "\nvoid main()\n{\n" + std::string{body} + "\n}\n";
}

const std::string unlit_fragment_source = fragment_source(
    R"(uniform vec3 u_fallback_color;
out vec4 out_color;
)",
    R"(    out_color = vec4(linear_to_srgb(clamp(u_fallback_color, 0.0, 1.0)), 1.0);)");

const std::string lambert_fragment_source = fragment_source(
    R"(in vec3 v_normal;
uniform vec3 u_light_direction;
uniform vec3 u_light_color;
uniform float u_light_intensity;
uniform vec3 u_ambient_contribution;
uniform vec3 u_diffuse_color;
out vec4 out_color;
)",
    R"(    vec3 normal = normalize(v_normal);
    float diffuse_factor = max(dot(normal, -u_light_direction), 0.0);
    vec3 linear_color = u_ambient_contribution +
                        u_diffuse_color * u_light_color * diffuse_factor * u_light_intensity;
    out_color = vec4(linear_to_srgb(clamp(linear_color, 0.0, 1.0)), 1.0);)");

const std::string phong_fragment_source = fragment_source(
    R"(in vec3 v_normal;
in vec3 v_world_position;
uniform vec3 u_light_direction;
uniform vec3 u_light_color;
uniform float u_light_intensity;
uniform vec3 u_ambient_contribution;
uniform vec3 u_diffuse_color;
uniform vec3 u_specular_color;
uniform float u_specular_power;
uniform vec3 u_eye_position;
out vec4 out_color;
)",
    R"(    vec3 normal = normalize(v_normal);
    float diffuse_factor = max(dot(normal, -u_light_direction), 0.0);
    vec3 linear_color = u_ambient_contribution +
                        u_diffuse_color * u_light_color * diffuse_factor * u_light_intensity;
    if (diffuse_factor > 0.0)
    {
        vec3 reflected = reflect(u_light_direction, normal);
        vec3 to_eye = normalize(u_eye_position - v_world_position);
        float highlight = pow(max(dot(reflected, to_eye), 0.0), u_specular_power);
        linear_color += u_specular_color * u_light_color * highlight * u_light_intensity;
    }
    out_color = vec4(linear_to_srgb(clamp(linear_color, 0.0, 1.0)), 1.0);)");

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

struct ViewportRenderer::UnlitProgram
{
    UnlitProgram()
        : program{"unlit fallback", unlit_vertex_source, unlit_fragment_source.c_str()},
          mvp{program.required_uniform("u_mvp")},
          fallback_color{program.required_uniform("u_fallback_color")}
    {
    }
    GlesProgram program;
    int mvp;
    int fallback_color;
};

struct ViewportRenderer::LambertProgram
{
    LambertProgram()
        : program{"Lambert", lit_vertex_source, lambert_fragment_source.c_str()},
          mvp{program.required_uniform("u_mvp")}, model{program.required_uniform("u_model")},
          light_direction{program.required_uniform("u_light_direction")},
          light_color{program.required_uniform("u_light_color")},
          light_intensity{program.required_uniform("u_light_intensity")},
          ambient{program.required_uniform("u_ambient_contribution")},
          diffuse{program.required_uniform("u_diffuse_color")}
    {
    }
    GlesProgram program;
    int mvp;
    int model;
    int light_direction;
    int light_color;
    int light_intensity;
    int ambient;
    int diffuse;
};

struct ViewportRenderer::PhongProgram
{
    PhongProgram()
        : program{"Phong", lit_vertex_source, phong_fragment_source.c_str()},
          mvp{program.required_uniform("u_mvp")}, model{program.required_uniform("u_model")},
          light_direction{program.required_uniform("u_light_direction")},
          light_color{program.required_uniform("u_light_color")},
          light_intensity{program.required_uniform("u_light_intensity")},
          ambient{program.required_uniform("u_ambient_contribution")},
          diffuse{program.required_uniform("u_diffuse_color")},
          specular{program.required_uniform("u_specular_color")},
          specular_power{program.required_uniform("u_specular_power")},
          eye_position{program.required_uniform("u_eye_position")}
    {
    }
    GlesProgram program;
    int mvp;
    int model;
    int light_direction;
    int light_color;
    int light_intensity;
    int ambient;
    int diffuse;
    int specular;
    int specular_power;
    int eye_position;
};

struct ViewportRenderer::HelperProgram
{
    HelperProgram()
        : program{"viewport helpers", helper_vertex_source, helper_fragment_source},
          view_projection{program.required_uniform("u_view_projection")},
          depth_bias{program.required_uniform("u_depth_bias")}
    {
    }
    GlesProgram program;
    int view_projection;
    int depth_bias;
};

ViewportRenderer::ViewportRenderer()
{
    unlit_program_ = std::make_unique<UnlitProgram>();
    lambert_program_ = std::make_unique<LambertProgram>();
    phong_program_ = std::make_unique<PhongProgram>();
    helper_program_ = std::make_unique<HelperProgram>();
    glGenVertexArrays(1, &helper_vertex_array_);
    glGenBuffers(1, &helper_vertex_buffer_);
    if (helper_vertex_array_ == 0 || helper_vertex_buffer_ == 0)
    {
        if (helper_vertex_buffer_ != 0)
            glDeleteBuffers(1, &helper_vertex_buffer_);
        if (helper_vertex_array_ != 0)
            glDeleteVertexArrays(1, &helper_vertex_array_);
        throw std::runtime_error("Viewport helper resource creation failed");
    }
    gl_description_ =
        gl_string(GL_VERSION) + " | " + gl_string(GL_VENDOR) + " | " + gl_string(GL_RENDERER);
    require_no_gl_error("Viewport renderer initialization");
}

ViewportRenderer::~ViewportRenderer()
{
    clear_geometry_cache();
    destroy_render_target();
    if (helper_vertex_buffer_ != 0)
        glDeleteBuffers(1, &helper_vertex_buffer_);
    if (helper_vertex_array_ != 0)
        glDeleteVertexArrays(1, &helper_vertex_array_);
}

void ViewportRenderer::destroy_geometry(PrimitiveGeometry& geometry)
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
    for (auto& entry : geometry_cache_)
        destroy_geometry(entry.second);
    geometry_cache_.clear();
}

void ViewportRenderer::synchronize_geometry_cache(const EditorState& scene)
{
    std::unordered_set<ObjectId> live_ids;
    for (const SceneObject& object : scene.objects())
        live_ids.insert(object.id);
    for (auto it = geometry_cache_.begin(); it != geometry_cache_.end();)
    {
        if (live_ids.count(it->first) != 0)
        {
            ++it;
            continue;
        }
        destroy_geometry(it->second);
        it = geometry_cache_.erase(it);
    }
}

ViewportRenderer::PrimitiveGeometry& ViewportRenderer::primitive_geometry(const SceneObject& object)
{
    auto [it, inserted] = geometry_cache_.try_emplace(object.id);
    PrimitiveGeometry& geometry = it->second;
    if (!inserted && geometry.primitive_kind == object.primitive_kind &&
        ((object.primitive_kind == PrimitiveKind::sphere &&
          geometry.radius_meters == object.sphere.radius_meters) ||
         (object.primitive_kind == PrimitiveKind::box &&
          geometry.box.width_meters == object.box.width_meters &&
          geometry.box.length_meters == object.box.length_meters &&
          geometry.box.height_meters == object.box.height_meters &&
          geometry.box.width_segments == object.box.width_segments &&
          geometry.box.length_segments == object.box.length_segments &&
          geometry.box.height_segments == object.box.height_segments)))
        return geometry;

    destroy_geometry(geometry);
    const TriangleMesh mesh = object.primitive_kind == PrimitiveKind::sphere
                                  ? make_sphere_mesh(object.sphere.radius_meters)
                                  : make_box_mesh(object.box);
    geometry.radius_meters = object.sphere.radius_meters;
    geometry.box = object.box;
    geometry.primitive_kind = object.primitive_kind;
    geometry.index_count = static_cast<std::uint32_t>(mesh.indices.size());
    glGenVertexArrays(1, &geometry.vertex_array);
    glGenBuffers(1, &geometry.vertex_buffer);
    glGenBuffers(1, &geometry.index_buffer);
    if (geometry.vertex_array == 0 || geometry.vertex_buffer == 0 || geometry.index_buffer == 0)
    {
        destroy_geometry(geometry);
        throw std::runtime_error("Viewport primitive resource creation failed");
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

void ViewportRenderer::render(const EditorState& scene, const ResolvedViewportView& view,
                              RenderTargetSize size, ViewportHelperInputs helpers)
{
    synchronize_geometry_cache(scene);
    if (requires_render_target_resize(size_, size))
        resize(size);
    if (size_.width == 0 || size_.height == 0)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, size_.width, size_.height);
    apply_depth_state(viewport_scene_depth_state);
    glDepthFunc(GL_LESS);
    glClearColor(0.055F, 0.07F, 0.10F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const glm::mat4 view_projection = view.projection * view.view;
    glm::vec3 light_direction{0.0F, 0.0F, -1.0F};
    glm::vec3 light_color{1.0F};
    float light_intensity = 0.0F;
    const auto lights = scene.lights(LightKind::directional, {true, false});
    if (!lights.empty())
    {
        light_direction = directional_light_direction(scene, lights.front()->id);
        light_color = lights.front()->directional_light.color;
        light_intensity = lights.front()->directional_light.intensity;
    }
    std::vector<const SceneObject*> renderables =
        scene.primitives(PrimitiveKind::sphere, {true, true});
    const auto boxes = scene.primitives(PrimitiveKind::box, {true, true});
    renderables.insert(renderables.end(), boxes.begin(), boxes.end());
    for (const SceneObject* object : renderables)
    {
        const PrimitiveGeometry& geometry = primitive_geometry(*object);
        glBindVertexArray(geometry.vertex_array);
        const glm::mat4 model = scene.world_transform_matrix(object->id);
        const glm::mat4 mvp = view_projection * model;
        const MaterialId material_id = object->primitive_kind == PrimitiveKind::sphere
                                           ? object->sphere.material_id
                                           : object->box.material_id;
        const glm::vec3 fallback = object->primitive_kind == PrimitiveKind::sphere
                                       ? object->sphere.fallback_color
                                       : object->box.fallback_color;
        const Material* material = scene.find_material(material_id);
        if (material == nullptr)
        {
            const UnlitProgram& program = *unlit_program_;
            glUseProgram(program.program.id());
            glUniformMatrix4fv(program.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform3fv(program.fallback_color, 1, glm::value_ptr(fallback));
        }
        else if (material->shading == MaterialShading::lambert)
        {
            const LambertProgram& program = *lambert_program_;
            glUseProgram(program.program.id());
            glUniformMatrix4fv(program.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(program.model, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(program.light_direction, 1, glm::value_ptr(light_direction));
            glUniform3fv(program.light_color, 1, glm::value_ptr(light_color));
            glUniform1f(program.light_intensity, light_intensity);
            glUniform3fv(program.ambient, 1, glm::value_ptr(material->ambient_color));
            glUniform3fv(program.diffuse, 1, glm::value_ptr(material->diffuse_color));
        }
        else if (material->shading == MaterialShading::phong)
        {
            const PhongProgram& program = *phong_program_;
            glUseProgram(program.program.id());
            glUniformMatrix4fv(program.mvp, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniformMatrix4fv(program.model, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(program.light_direction, 1, glm::value_ptr(light_direction));
            glUniform3fv(program.light_color, 1, glm::value_ptr(light_color));
            glUniform1f(program.light_intensity, light_intensity);
            glUniform3fv(program.ambient, 1, glm::value_ptr(material->ambient_color));
            glUniform3fv(program.diffuse, 1, glm::value_ptr(material->diffuse_color));
            glUniform3fv(program.specular, 1, glm::value_ptr(material->specular_color));
            glUniform1f(program.specular_power, material->specular_power);
            glUniform3fv(program.eye_position, 1, glm::value_ptr(view.eye_position));
        }
        else
            throw std::runtime_error("Viewport material shading type is invalid");
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(geometry.index_count), GL_UNSIGNED_INT,
                       nullptr);
    }
    if (helpers.bounds != nullptr)
        render_helpers(*helpers.bounds, view_projection, HelperRenderRole::bounds);
    if (helpers.gizmo != nullptr)
    {
        const ResolvedViewportView& gizmo_view =
            helpers.gizmo_view == nullptr ? view : *helpers.gizmo_view;
        render_helpers(*helpers.gizmo, gizmo_view.projection * gizmo_view.view,
                       HelperRenderRole::gizmo);
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    require_no_gl_error("Viewport scene render");
}

void ViewportRenderer::render_helpers(const HelperGeometry& helpers,
                                      const glm::mat4& view_projection, HelperRenderRole role)
{
    if (helpers.lines.empty() && helpers.triangles.empty())
        return;
    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 color;
    };
    std::vector<Vertex> vertices;
    vertices.reserve(helpers.lines.size() * 2 + helpers.triangles.size() * 3);
    for (const ColoredLine& line : helpers.lines)
    {
        vertices.push_back({line.start, line.color});
        vertices.push_back({line.end, line.color});
    }
    const std::size_t line_vertices = vertices.size();
    for (const ColoredTriangle& triangle : helpers.triangles)
    {
        vertices.push_back({triangle.first, triangle.color});
        vertices.push_back({triangle.second, triangle.color});
        vertices.push_back({triangle.third, triangle.color});
    }
    const HelperProgram& program = *helper_program_;
    const HelperDepthState depth = helper_depth_state(role);
    apply_depth_state(depth);
    if (depth.depth_test_enabled)
        glDepthFunc(GL_LESS);
    glUseProgram(program.program.id());
    glUniformMatrix4fv(program.view_projection, 1, GL_FALSE, glm::value_ptr(view_projection));
    glUniform1f(program.depth_bias, depth.depth_bias_enabled ? helper_depth_bias : 0.0F);
    glBindVertexArray(helper_vertex_array_);
    glBindBuffer(GL_ARRAY_BUFFER, helper_vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          reinterpret_cast<const void*>(offsetof(Vertex, color)));
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(line_vertices));
    glDrawArrays(GL_TRIANGLES, static_cast<GLint>(line_vertices),
                 static_cast<GLsizei>(vertices.size() - line_vertices));
    apply_depth_state(restored_helper_depth_state);
    glDepthFunc(GL_LESS);
}
} // namespace ai3
