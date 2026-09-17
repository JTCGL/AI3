#include "core/edit_operations.h"

#include <utility>

namespace ai3
{
namespace
{
template <typename Mutation> auto apply_authored(EditHistory& history, Mutation&& mutation)
{
    const bool owns_transaction = !history.transaction_active();
    if (owns_transaction)
        history.begin_transaction();
    try
    {
        auto result = mutation();
        if (owns_transaction)
            history.commit_transaction();
        return result;
    }
    catch (...)
    {
        if (owns_transaction)
            history.cancel_transaction();
        throw;
    }
}
} // namespace

ContinuousEdit::ContinuousEdit(EditHistory& history) : history_(&history)
{
    if (!history_->begin_transaction())
        history_ = nullptr;
}

ContinuousEdit::ContinuousEdit(ContinuousEdit&& other) noexcept : history_(other.history_)
{
    other.history_ = nullptr;
}

ContinuousEdit& ContinuousEdit::operator=(ContinuousEdit&& other) noexcept
{
    if (this == &other)
        return *this;
    cancel();
    history_ = other.history_;
    other.history_ = nullptr;
    return *this;
}

ContinuousEdit::~ContinuousEdit() { cancel(); }

bool ContinuousEdit::active() const
{
    return history_ != nullptr && history_->transaction_active();
}

bool ContinuousEdit::commit()
{
    if (!active())
        return false;
    EditHistory* history = history_;
    history_ = nullptr;
    return history->commit_transaction();
}

bool ContinuousEdit::cancel()
{
    if (!active())
    {
        history_ = nullptr;
        return false;
    }
    EditHistory* history = history_;
    history_ = nullptr;
    return history->cancel_transaction();
}

EditOperations::EditOperations(Scene& scene, Workspace& workspace, EditHistory& history)
    : scene_(scene), workspace_(workspace), history_(history)
{
}

ContinuousEdit EditOperations::begin_continuous_edit() { return ContinuousEdit{history_}; }

ObjectId EditOperations::create_object(CreateObject object)
{
    return apply_authored(history_, [&] { return scene_.create_object(std::move(object)); });
}
ObjectId EditOperations::create_sphere(std::string name, SpherePrimitive sphere)
{
    return apply_authored(history_, [&] { return scene_.create_sphere(std::move(name), sphere); });
}
ObjectId EditOperations::create_box(std::string name, BoxPrimitive box)
{
    return apply_authored(history_, [&] { return scene_.create_box(std::move(name), box); });
}
ObjectId EditOperations::create_perspective_camera(std::string name, PerspectiveCamera camera)
{
    return apply_authored(history_, [&]
                          { return scene_.create_perspective_camera(std::move(name), camera); });
}
ObjectId EditOperations::create_directional_light(std::string name, DirectionalLight light)
{
    return apply_authored(history_,
                          [&] { return scene_.create_directional_light(std::move(name), light); });
}
bool EditOperations::set_sphere(ObjectId id, SpherePrimitive sphere)
{
    return apply_authored(history_, [&] { return scene_.set_sphere(id, sphere); });
}
bool EditOperations::set_box(ObjectId id, BoxPrimitive box)
{
    return apply_authored(history_, [&] { return scene_.set_box(id, box); });
}
bool EditOperations::set_perspective_camera(ObjectId id, PerspectiveCamera camera)
{
    return apply_authored(history_, [&] { return scene_.set_perspective_camera(id, camera); });
}
bool EditOperations::set_directional_light(ObjectId id, DirectionalLight light)
{
    return apply_authored(history_, [&] { return scene_.set_directional_light(id, light); });
}
MaterialId EditOperations::create_material(std::string name, Material material)
{
    return apply_authored(history_, [&]
                          { return scene_.create_material(std::move(name), std::move(material)); });
}
bool EditOperations::rename_material(MaterialId id, std::string name)
{
    return apply_authored(history_, [&] { return scene_.rename_material(id, std::move(name)); });
}
bool EditOperations::set_material(MaterialId id, Material material)
{
    return apply_authored(history_, [&] { return scene_.set_material(id, std::move(material)); });
}
bool EditOperations::assign_material(ObjectId id, MaterialId material)
{
    return apply_authored(history_, [&] { return scene_.assign_material(id, material); });
}
bool EditOperations::set_sphere_fallback_color(ObjectId id, glm::vec3 color)
{
    return apply_authored(history_, [&] { return scene_.set_sphere_fallback_color(id, color); });
}
bool EditOperations::rename_object(ObjectId id, std::string name)
{
    return apply_authored(history_, [&] { return scene_.rename_object(id, std::move(name)); });
}
bool EditOperations::set_object_enabled(ObjectId id, bool enabled)
{
    return apply_authored(history_, [&] { return scene_.set_object_enabled(id, enabled); });
}
bool EditOperations::set_object_visible(ObjectId id, bool visible)
{
    return apply_authored(history_, [&] { return scene_.set_object_visible(id, visible); });
}
bool EditOperations::set_local_transform(ObjectId id, Transform transform)
{
    return apply_authored(history_,
                          [&] { return scene_.set_local_transform(id, std::move(transform)); });
}
bool EditOperations::set_world_position(ObjectId id, glm::vec3 world_position)
{
    return apply_authored(history_, [&] { return scene_.set_world_position(id, world_position); });
}
bool EditOperations::reparent_object(ObjectId id, ObjectId new_parent)
{
    return apply_authored(history_, [&] { return scene_.reparent_object(id, new_parent); });
}
bool EditOperations::delete_object(ObjectId id)
{
    return apply_authored(history_,
                          [&]
                          {
                              if (!scene_.delete_object(id))
                                  return false;
                              workspace_.remove_bounds_display(id);
                              if (workspace_.selection() == id)
                                  workspace_.clear_selection();
                              return true;
                          });
}
bool EditOperations::reset_scene()
{
    return apply_authored(history_,
                          [&]
                          {
                              const bool changed = scene_.reset_scene();
                              workspace_.clear_selection();
                              workspace_.replace_bounds_display({});
                              return changed;
                          });
}

bool EditOperations::select(ObjectId id)
{
    if (scene_.find_object(id) == nullptr || workspace_.selection() == id)
        return false;
    workspace_.set_selection(id);
    return true;
}
void EditOperations::clear_selection() { workspace_.clear_selection(); }
bool EditOperations::set_bounds_display(ObjectId id, BoundsDisplayState display)
{
    const SceneObject* object = scene_.find_object(id);
    if (object == nullptr || !object->bounds.box.has_value() || !object->bounds.sphere.has_value())
        return false;
    workspace_.set_bounds_display(id, display);
    return true;
}
void EditOperations::replace_bounds_display(std::map<ObjectId, BoundsDisplayState> display)
{
    std::map<ObjectId, BoundsDisplayState> validated;
    for (const auto& [id, state] : display)
    {
        const SceneObject* object = scene_.find_object(id);
        if (object != nullptr && object->bounds.box.has_value() &&
            object->bounds.sphere.has_value() &&
            (state.show_bounding_box || state.show_bounding_sphere || state.hover_feedback))
            validated.emplace(id, state);
    }
    workspace_.replace_bounds_display(std::move(validated));
}
} // namespace ai3
