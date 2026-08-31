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
           left.perspective_camera.vertical_fov_degrees ==
               right.perspective_camera.vertical_fov_degrees &&
           left.perspective_camera.near_plane_meters ==
               right.perspective_camera.near_plane_meters &&
           left.perspective_camera.far_plane_meters == right.perspective_camera.far_plane_meters &&
           equal(left.directional_light.color, right.directional_light.color) &&
           left.directional_light.intensity == right.directional_light.intensity;
}
} // namespace

EditorHistory::EditorHistory(EditorState& state) : state_(state) {}

EditorHistory::Snapshot EditorHistory::capture() const
{
    return {state_.objects_, state_.next_object_id_, state_.default_name_counts_};
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
    const bool unchanged =
        transaction_before_.next_object_id == after.next_object_id &&
        transaction_before_.default_name_counts == after.default_name_counts &&
        transaction_before_.objects.size() == after.objects.size() &&
        std::equal(transaction_before_.objects.begin(), transaction_before_.objects.end(),
                   after.objects.begin(), [](const SceneObject& left, const SceneObject& right)
                   { return equal(left, right); });
    if (unchanged)
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
    const bool unchanged =
        current.next_object_id == snapshot.next_object_id &&
        current.default_name_counts == snapshot.default_name_counts &&
        current.objects.size() == snapshot.objects.size() &&
        std::equal(current.objects.begin(), current.objects.end(), snapshot.objects.begin(),
                   [](const SceneObject& left, const SceneObject& right)
                   { return equal(left, right); });
    if (unchanged)
        return;
    state_.objects_ = snapshot.objects;
    state_.next_object_id_ = snapshot.next_object_id;
    state_.default_name_counts_ = snapshot.default_name_counts;
    if (state_.selection_ != no_object && state_.find_object(state_.selection_) == nullptr)
        state_.selection_ = no_object;
    state_.advance_document_revision();
}

HistoryStateId EditorHistory::allocate_state_id()
{
    if (next_state_id_ == std::numeric_limits<HistoryStateId>::max())
        throw std::overflow_error("Editor history state identity exhausted");
    return next_state_id_++;
}
} // namespace ai3
