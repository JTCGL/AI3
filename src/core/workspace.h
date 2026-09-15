#pragma once

#include "core/length_units.h"
#include "core/scene.h"

#include <map>

namespace ai3
{
struct BoundsDisplayState
{
    bool show_bounding_box = false;
    bool show_bounding_sphere = false;
    bool hover_feedback = false;
};

class Workspace
{
    public:
    ObjectId selection() const;
    void set_selection(ObjectId id);
    void clear_selection();

    const BoundsDisplayState& bounds_display(ObjectId id) const;
    void set_bounds_display(ObjectId id, BoundsDisplayState display);
    void remove_bounds_display(ObjectId id);
    void replace_bounds_display(std::map<ObjectId, BoundsDisplayState> display);
    const std::map<ObjectId, BoundsDisplayState>& bounds_display_states() const;

    MaterialId active_material() const;
    void set_active_material(MaterialId id);
    void clear_active_material();

    LengthUnit display_length_unit() const;
    void set_display_length_unit(LengthUnit unit);

    private:
    ObjectId selection_ = no_object;
    std::map<ObjectId, BoundsDisplayState> bounds_display_;
    MaterialId active_material_ = no_material;
    LengthUnit display_length_unit_ = default_display_length_unit;
};
} // namespace ai3
