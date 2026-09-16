#include "core/edit_history.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ai3
{
namespace
{
bool equal(const glm::vec3& left, const glm::vec3& right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

bool equal(const glm::quat& left, const glm::quat& right)
{
    return left.w == right.w && left.x == right.x && left.y == right.y && left.z == right.z;
}

bool equal(const Transform& left, const Transform& right)
{
    return equal(left.position, right.position) && equal(left.orientation, right.orientation) &&
           equal(left.scale, right.scale);
}

bool equal(const SceneObject& left, const SceneObject& right)
{
    return left.id == right.id && left.name == right.name && left.enabled == right.enabled &&
           left.visible == right.visible && left.parent_id() == right.parent_id() &&
           equal(left.transform, right.transform) && left.category == right.category &&
           left.primitive_kind == right.primitive_kind && left.camera_kind == right.camera_kind &&
           left.light_kind == right.light_kind &&
           left.sphere.radius_meters == right.sphere.radius_meters &&
           left.sphere.material_id == right.sphere.material_id &&
           equal(left.sphere.fallback_color, right.sphere.fallback_color) &&
           left.box.width_meters == right.box.width_meters &&
           left.box.length_meters == right.box.length_meters &&
           left.box.height_meters == right.box.height_meters &&
           left.box.width_segments == right.box.width_segments &&
           left.box.length_segments == right.box.length_segments &&
           left.box.height_segments == right.box.height_segments &&
           left.box.material_id == right.box.material_id &&
           equal(left.box.fallback_color, right.box.fallback_color) &&
           left.perspective_camera.vertical_fov_degrees ==
               right.perspective_camera.vertical_fov_degrees &&
           left.perspective_camera.near_plane_meters ==
               right.perspective_camera.near_plane_meters &&
           left.perspective_camera.far_plane_meters == right.perspective_camera.far_plane_meters &&
           equal(left.directional_light.color, right.directional_light.color) &&
           left.directional_light.intensity == right.directional_light.intensity;
}
bool equal(const Material& left, const Material& right)
{
    return left.id == right.id && left.name == right.name && left.shading == right.shading &&
           equal(left.ambient_color, right.ambient_color) &&
           equal(left.diffuse_color, right.diffuse_color) &&
           equal(left.specular_color, right.specular_color) &&
           left.specular_power == right.specular_power;
}
} // namespace

EditHistory::EditHistory(Scene& scene, Workspace& workspace) : scene_(scene), workspace_(workspace)
{
}

EditHistory::Snapshot EditHistory::capture() const
{
    return {scene_, workspace_.bounds_display_states()};
}

bool EditHistory::begin_transaction()
{
    if (transaction_active_)
        return false;
    transaction_before_ = capture();
    transaction_active_ = true;
    return true;
}

bool EditHistory::commit_transaction()
{
    if (!transaction_active_)
        return false;
    Snapshot after = capture();
    transaction_active_ = false;
    if (snapshots_equal(transaction_before_, after))
        return false;
    entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(position_), entries_.end());
    const HistoryStateId before_id = current_state_id();
    const HistoryStateId after_id = allocate_state_id();
    std::map<ObjectId, BoundsDisplayState> deleted_object_workspace;
    const bool reset_scene = after.scene.objects_.empty() && after.scene.next_object_id_ == 1 &&
                             after.scene.next_material_id_ == 1 &&
                             after.scene.default_name_counts_.empty() &&
                             after.scene.default_material_name_count_ == 0;
    if (!reset_scene)
        for (const SceneObject& object : transaction_before_.scene.objects_)
            if (std::none_of(after.scene.objects_.begin(), after.scene.objects_.end(),
                             [id = object.id](const SceneObject& candidate)
                             { return candidate.id == id; }))
            {
                const auto display = transaction_before_.object_lifecycle_bounds.find(object.id);
                deleted_object_workspace.emplace(
                    object.id, display == transaction_before_.object_lifecycle_bounds.end()
                                   ? BoundsDisplayState{}
                                   : display->second);
            }
    entries_.push_back({std::move(transaction_before_), std::move(after), before_id, after_id,
                        std::move(deleted_object_workspace)});
    position_ = entries_.size();
    return true;
}

bool EditHistory::cancel_transaction()
{
    if (!transaction_active_)
        return false;
    std::map<ObjectId, BoundsDisplayState> removed_object_workspace;
    for (const auto& [id, display] : transaction_before_.object_lifecycle_bounds)
        if (scene_.find_object(id) == nullptr)
            removed_object_workspace.emplace(id, display);
    Snapshot before = std::move(transaction_before_);
    transaction_active_ = false;
    restore(before);
    for (const auto& [id, display] : removed_object_workspace)
        if (scene_.find_object(id) != nullptr)
            workspace_.set_bounds_display(id, display);
    return true;
}

bool EditHistory::transaction_active() const { return transaction_active_; }
bool EditHistory::has_uncommitted_changes() const
{
    return transaction_active_ && !snapshots_equal(transaction_before_, capture());
}
bool EditHistory::can_undo() const { return !transaction_active_ && position_ > 0; }
bool EditHistory::can_redo() const { return !transaction_active_ && position_ < entries_.size(); }

bool EditHistory::undo()
{
    if (!can_undo())
        return false;
    const Entry& entry = entries_[position_ - 1];
    restore(entry.before);
    for (const auto& [id, display] : entry.deleted_object_workspace)
        if (scene_.find_object(id) != nullptr)
            workspace_.set_bounds_display(id, display);
    --position_;
    return true;
}

bool EditHistory::redo()
{
    if (!can_redo())
        return false;
    Entry& entry = entries_[position_];
    for (auto& [id, display] : entry.deleted_object_workspace)
        display = workspace_.bounds_display(id);
    restore(entry.after);
    for (const auto& [id, display] : entry.deleted_object_workspace)
        workspace_.remove_bounds_display(id);
    ++position_;
    return true;
}

HistoryStateId EditHistory::current_state_id() const
{
    return position_ == 0 ? baseline_id_ : entries_[position_ - 1].after_id;
}

void EditHistory::rebaseline()
{
    transaction_active_ = false;
    entries_.clear();
    position_ = 0;
    baseline_id_ = allocate_state_id();
}

void EditHistory::restore(const Snapshot& snapshot)
{
    const Snapshot current = capture();
    if (snapshots_equal(current, snapshot))
        return;
    const DocumentRevision revision = scene_.document_revision_;
    scene_ = snapshot.scene;
    scene_.document_revision_ = revision;
    if (workspace_.selection() != no_object &&
        scene_.find_object(workspace_.selection()) == nullptr)
        workspace_.clear_selection();
    scene_.advance_document_revision();
}

bool EditHistory::snapshots_equal(const Snapshot& left, const Snapshot& right)
{
    return left.scene.next_object_id_ == right.scene.next_object_id_ &&
           left.scene.next_material_id_ == right.scene.next_material_id_ &&
           left.scene.default_material_name_count_ == right.scene.default_material_name_count_ &&
           left.scene.default_name_counts_ == right.scene.default_name_counts_ &&
           left.scene.materials_.size() == right.scene.materials_.size() &&
           std::equal(left.scene.materials_.begin(), left.scene.materials_.end(),
                      right.scene.materials_.begin(),
                      [](const Material& a, const Material& b) { return equal(a, b); }) &&
           left.scene.objects_.size() == right.scene.objects_.size() &&
           std::equal(left.scene.objects_.begin(), left.scene.objects_.end(),
                      right.scene.objects_.begin(),
                      [](const SceneObject& left_object, const SceneObject& right_object)
                      { return equal(left_object, right_object); });
}

HistoryStateId EditHistory::allocate_state_id()
{
    if (next_state_id_ == std::numeric_limits<HistoryStateId>::max())
        throw std::overflow_error("Edit history state identity exhausted");
    return next_state_id_++;
}
} // namespace ai3
