#pragma once

#include <glm/vec3.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace ai3
{
using ObjectId = std::uint32_t;
constexpr ObjectId no_object = 0;

struct Transform
{
    glm::vec3 position{0.0F};
    glm::vec3 rotation{0.0F};
    glm::vec3 scale{1.0F};
};

enum class RenderableKind
{
    none,
    cube
};

struct SceneObject
{
    ObjectId id = no_object;
    ObjectId parent = no_object;
    std::string name;
    std::string type;
    bool enabled = true;
    bool visible = true;
    Transform transform;
    RenderableKind renderable = RenderableKind::none;
};

struct ConsoleMessage
{
    std::string key;
    std::string argument;
};

enum class EditorPanel : std::size_t
{
    scene_graph,
    viewport,
    object_inspector,
    console,
    count
};

class EditorState
{
    public:
    EditorState();

    const std::vector<SceneObject>& objects() const;
    SceneObject* find_object(ObjectId id);
    const SceneObject* find_object(ObjectId id) const;
    std::vector<ObjectId> children_of(ObjectId parent) const;

    ObjectId selection() const;
    bool select(ObjectId id);
    void clear_selection();

    bool panel_visible(EditorPanel panel) const;
    void set_panel_visible(EditorPanel panel, bool visible);

    const std::vector<ConsoleMessage>& console_messages() const;
    void add_console_message(std::string key, std::string argument = {});
    void clear_console();

    void request_layout_reset();
    bool consume_layout_reset_request();

    private:
    std::vector<SceneObject> objects_;
    ObjectId selection_ = no_object;
    std::array<bool, static_cast<std::size_t>(EditorPanel::count)> panel_visibility_ = {true, true,
                                                                                        true, true};
    std::vector<ConsoleMessage> console_messages_;
    bool layout_reset_requested_ = false;
};
} // namespace ai3
