#include "core/workspace.h"

namespace ai3
{
ObjectId Workspace::selection() const { return selection_; }
void Workspace::set_selection(ObjectId id) { selection_ = id; }
void Workspace::clear_selection() { selection_ = no_object; }

const BoundsDisplayState& Workspace::bounds_display(ObjectId id) const
{
    static const BoundsDisplayState defaults;
    const auto found = bounds_display_.find(id);
    return found == bounds_display_.end() ? defaults : found->second;
}

void Workspace::set_bounds_display(ObjectId id, BoundsDisplayState display)
{
    if (!display.show_bounding_box && !display.show_bounding_sphere && !display.hover_feedback)
        bounds_display_.erase(id);
    else
        bounds_display_[id] = display;
}

void Workspace::remove_bounds_display(ObjectId id) { bounds_display_.erase(id); }

void Workspace::replace_bounds_display(std::map<ObjectId, BoundsDisplayState> display)
{
    bounds_display_.clear();
    for (const auto& [id, state] : display)
        set_bounds_display(id, state);
}

const std::map<ObjectId, BoundsDisplayState>& Workspace::bounds_display_states() const
{
    return bounds_display_;
}

MaterialId Workspace::active_material() const { return active_material_; }
void Workspace::set_active_material(MaterialId id) { active_material_ = id; }
void Workspace::clear_active_material() { active_material_ = no_material; }

LengthUnit Workspace::display_length_unit() const { return display_length_unit_; }
void Workspace::set_display_length_unit(LengthUnit unit) { display_length_unit_ = unit; }

ViewportState& Workspace::viewport() { return viewport_; }
const ViewportState& Workspace::viewport() const { return viewport_; }

void Workspace::transition_document()
{
    clear_selection();
    replace_bounds_display({});
    clear_active_material();
    viewport_.source = ViewSource::editor_view;
    viewport_.scene_camera_id = no_object;
}
} // namespace ai3
