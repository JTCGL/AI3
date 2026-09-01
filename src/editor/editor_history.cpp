#include "editor/editor_history.h"

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

EditorHistory::EditorHistory(EditorState& state) : state_(state) {}

EditorHistory::Snapshot EditorHistory::capture() const
{
    return {state_.objects_,
            state_.materials_,
            state_.next_object_id_,
            state_.next_material_id_,
            state_.default_material_name_count_,
            state_.default_name_counts_};
}

bool EditorHistory::begin_transaction()
{
    if (transaction_active_)
        return false;
    transaction_before_ = capture();
    transaction_active_ = true;
    return true;
}

bool EditorHistory::commit_transaction()
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
    entries_.push_back({std::move(transaction_before_), std::move(after), before_id, after_id});
    position_ = entries_.size();
    return true;
}

bool EditorHistory::cancel_transaction()
{
    if (!transaction_active_)
        return false;
    Snapshot before = std::move(transaction_before_);
    transaction_active_ = false;
    restore(before);
    return true;
}

bool EditorHistory::transaction_active() const { return transaction_active_; }
bool EditorHistory::has_uncommitted_changes() const
{
    return transaction_active_ && !snapshots_equal(transaction_before_, capture());
}
bool EditorHistory::can_undo() const { return !transaction_active_ && position_ > 0; }
bool EditorHistory::can_redo() const { return !transaction_active_ && position_ < entries_.size(); }

bool EditorHistory::undo()
{
    if (!can_undo())
        return false;
    restore(entries_[position_ - 1].before);
    --position_;
    return true;
}

bool EditorHistory::redo()
{
    if (!can_redo())
        return false;
    restore(entries_[position_].after);
    ++position_;
    return true;
}

HistoryStateId EditorHistory::current_state_id() const
{
    return position_ == 0 ? baseline_id_ : entries_[position_ - 1].after_id;
}

void EditorHistory::rebaseline()
{
    transaction_active_ = false;
    entries_.clear();
    position_ = 0;
    baseline_id_ = allocate_state_id();
}

void EditorHistory::restore(const Snapshot& snapshot)
{
    const Snapshot current = capture();
    if (snapshots_equal(current, snapshot))
        return;
    state_.objects_ = snapshot.objects;
    state_.materials_ = snapshot.materials;
    state_.next_object_id_ = snapshot.next_object_id;
    state_.next_material_id_ = snapshot.next_material_id;
    state_.default_material_name_count_ = snapshot.default_material_name_count;
    state_.default_name_counts_ = snapshot.default_name_counts;
    if (state_.selection_ != no_object && state_.find_object(state_.selection_) == nullptr)
        state_.selection_ = no_object;
    state_.advance_document_revision();
}

bool EditorHistory::snapshots_equal(const Snapshot& left, const Snapshot& right)
{
    return left.next_object_id == right.next_object_id &&
           left.next_material_id == right.next_material_id &&
           left.default_material_name_count == right.default_material_name_count &&
           left.default_name_counts == right.default_name_counts &&
           left.materials.size() == right.materials.size() &&
           std::equal(left.materials.begin(), left.materials.end(), right.materials.begin(),
                      [](const Material& a, const Material& b) { return equal(a, b); }) &&
           left.objects.size() == right.objects.size() &&
           std::equal(left.objects.begin(), left.objects.end(), right.objects.begin(),
                      [](const SceneObject& left_object, const SceneObject& right_object)
                      { return equal(left_object, right_object); });
}

HistoryStateId EditorHistory::allocate_state_id()
{
    if (next_state_id_ == std::numeric_limits<HistoryStateId>::max())
        throw std::overflow_error("Editor history state identity exhausted");
    return next_state_id_++;
}
} // namespace ai3
