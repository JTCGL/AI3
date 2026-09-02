#include <doctest/doctest.h>

#include "editor/document_session.h"
#include "scene/scene_math.h"
#include "scene/translation_gizmo.h"
#include "scene/viewport_view.h"

#include <glm/geometric.hpp>

#include <limits>

namespace
{
void check_vec3(glm::vec3 actual, glm::vec3 expected)
{
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(0.0001F));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(0.0001F));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(0.0001F));
}

ai3::ObjectId create_child(ai3::EditorState& state, ai3::Transform parent_transform)
{
    const ai3::ObjectId parent =
        state.create_object(ai3::CreateObject{"Parent", ai3::no_object, parent_transform});
    ai3::Transform child;
    child.position = {1.0F, 2.0F, 3.0F};
    child.orientation = ai3::orientation_from_euler_degrees({12.0F, 23.0F, 34.0F});
    child.scale = {2.0F, -3.0F, 4.0F};
    return state.create_object(ai3::CreateObject{"Child", parent, child});
}
} // namespace

TEST_CASE("world position mutation handles root and invertible parent transforms exactly")
{
    ai3::EditorState root_state;
    const ai3::ObjectId root = root_state.create_object(ai3::CreateObject{"Root"});
    REQUIRE(root_state.set_world_position(root, {4.0F, 5.0F, 6.0F}));
    check_vec3(root_state.find_object(root)->transform.position, {4.0F, 5.0F, 6.0F});

    for (const ai3::Transform parent_transform :
         {ai3::Transform{{5.0F, -2.0F, 1.0F}},
          ai3::Transform{{}, ai3::orientation_from_euler_degrees({0.0F, 0.0F, 90.0F})},
          ai3::Transform{{}, glm::quat{1.0F, 0.0F, 0.0F, 0.0F}, {2.0F, 2.0F, 2.0F}},
          ai3::Transform{
              {}, ai3::orientation_from_euler_degrees({10.0F, 20.0F, 30.0F}), {2.0F, 3.0F, 4.0F}},
          ai3::Transform{
              {}, ai3::orientation_from_euler_degrees({0.0F, 20.0F, 0.0F}), {-2.0F, 3.0F, 4.0F}}})
    {
        ai3::EditorState state;
        const ai3::ObjectId child = create_child(state, parent_transform);
        const ai3::Transform before = state.find_object(child)->transform;
        const ai3::ObjectId parent = state.find_object(child)->parent_id();
        REQUIRE(state.set_world_position(child, {7.0F, 8.0F, 9.0F}));
        check_vec3(state.world_position(child), {7.0F, 8.0F, 9.0F});
        CHECK(state.find_object(child)->parent_id() == parent);
        CHECK(state.find_object(child)->transform.orientation == before.orientation);
        CHECK(state.find_object(child)->transform.scale == before.scale);
    }
}

TEST_CASE("world position rejects singular invalid input and preserves semantic no-ops")
{
    ai3::EditorState state;
    ai3::Transform singular;
    singular.scale = {1.0F, 0.0F, 1.0F};
    const ai3::ObjectId child = create_child(state, singular);
    const ai3::Transform before = state.find_object(child)->transform;
    const ai3::DocumentRevision revision = state.document_revision();
    CHECK_FALSE(state.set_world_position(child, {8.0F, 9.0F, 10.0F}));
    CHECK_FALSE(
        state.set_world_position(child, {std::numeric_limits<float>::infinity(), 0.0F, 0.0F}));
    CHECK(state.find_object(child)->transform.position == before.position);
    CHECK(state.document_revision() == revision);

    ai3::EditorState root_state;
    const ai3::ObjectId root = root_state.create_object(ai3::CreateObject{"Root"});
    const ai3::DocumentRevision root_revision = root_state.document_revision();
    REQUIRE(root_state.set_world_position(root, root_state.world_position(root)));
    CHECK(root_state.document_revision() == root_revision);
}

TEST_CASE("axis constraints freeze start state and safely choose fallback")
{
    ai3::EditorState state;
    ai3::ViewportView viewport;
    const ai3::ResolvedViewportView view = viewport.resolve(state, 1.0F);
    const ai3::WorldRay start{{0.0F, 2.0F, 5.0F}, {0.0F, 0.0F, -1.0F}};
    const ai3::AxisDragConstraint normal =
        ai3::begin_axis_drag_constraint(start, {}, {1.0F, 0.0F, 0.0F}, view);
    REQUIRE(normal.valid);
    CHECK(normal.method == ai3::AxisConstraintMethod::closest_points);
    const auto first =
        ai3::constrained_axis_position(normal, {{3.0F, 2.0F, 5.0F}, {0.0F, 0.0F, -1.0F}});
    const auto repeated =
        ai3::constrained_axis_position(normal, {{3.0F, 2.0F, 5.0F}, {0.0F, 0.0F, -1.0F}});
    REQUIRE(first.has_value());
    REQUIRE(repeated.has_value());
    check_vec3(*first, {3.0F, 0.0F, 0.0F});
    check_vec3(*repeated, *first);

    const glm::vec3 forward = glm::normalize(glm::transpose(glm::mat3{view.view})[2]);
    const ai3::WorldRay near_parallel{{1.0F, 1.0F, 5.0F},
                                      glm::normalize(forward + glm::vec3{0.001F, 0.0F, 0.0F})};
    const ai3::AxisDragConstraint fallback =
        ai3::begin_axis_drag_constraint(near_parallel, {}, forward, view);
    CHECK(fallback.method == ai3::AxisConstraintMethod::view_fallback);
    REQUIRE(fallback.valid);
    REQUIRE(ai3::constrained_axis_position(fallback, near_parallel).has_value());

    ai3::WorldRay invalid = start;
    invalid.direction = {};
    CHECK_FALSE(ai3::begin_axis_drag_constraint(invalid, {}, {1.0F, 0.0F, 0.0F}, view).valid);
}

TEST_CASE("gizmo sizing projection and hit testing are display independent")
{
    ai3::EditorState state;
    ai3::ViewportView viewport;
    const ai3::ResolvedViewportView view = viewport.resolve(state, 1.0F);
    const auto near_length = ai3::translation_gizmo_world_length({}, view, 72.0F, 600.0F);
    const glm::vec3 farther = glm::normalize(view.eye_position) * -4.0F;
    const auto far_length = ai3::translation_gizmo_world_length(farther, view, 72.0F, 600.0F);
    REQUIRE(near_length.has_value());
    REQUIRE(far_length.has_value());
    CHECK(*far_length > *near_length);
    CHECK_FALSE(ai3::translation_gizmo_world_length({}, view, 0.0F, 600.0F).has_value());
    const auto pivot = ai3::project_world_to_viewport({}, view, {800.0F, 600.0F});
    REQUIRE(pivot.has_value());
    std::array<glm::vec2, 3> starts{*pivot, *pivot, *pivot};
    std::array<glm::vec2, 3> ends{*pivot + glm::vec2{50.0F, 0.0F}, *pivot + glm::vec2{0.0F, 50.0F},
                                  *pivot + glm::vec2{-40.0F, -40.0F}};
    CHECK(ai3::pick_translation_axis(*pivot + glm::vec2{30.0F, 2.0F}, starts, ends, 8.0F) ==
          ai3::TranslationAxis::x);
}

TEST_CASE("translation gesture is one history transaction with checkpoint semantics")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    REQUIRE(session.history().begin_transaction());
    const ai3::ObjectId object = state.create_object(ai3::CreateObject{"Object"});
    REQUIRE(session.history().commit_transaction());
    session.mark_saved();

    REQUIRE(session.history().begin_transaction());
    REQUIRE(state.set_world_position(object, {1.0F, 0.0F, 0.0F}));
    REQUIRE(state.set_world_position(object, {2.0F, 0.0F, 0.0F}));
    CHECK(session.dirty());
    REQUIRE(session.history().commit_transaction());
    REQUIRE(session.history().undo());
    CHECK_FALSE(session.dirty());
    check_vec3(state.world_position(object), {});
    REQUIRE(session.history().redo());
    CHECK(session.dirty());
    check_vec3(state.world_position(object), {2.0F, 0.0F, 0.0F});

    REQUIRE(session.history().begin_transaction());
    REQUIRE(state.set_world_position(object, state.world_position(object)));
    CHECK_FALSE(session.history().commit_transaction());
    REQUIRE(session.history().begin_transaction());
    REQUIRE(state.set_world_position(object, {5.0F, 0.0F, 0.0F}));
    REQUIRE(session.history().cancel_transaction());
    check_vec3(state.world_position(object), {2.0F, 0.0F, 0.0F});
}

TEST_CASE("reference space and transform tool choices remain workspace state")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    ai3::ViewportView viewport;
    const ai3::DocumentRevision revision = state.document_revision();
    const ai3::HistoryStateId history = session.history().current_state_id();
    viewport.set_transform_tool(ai3::ViewportTransformTool::translation);
    for (ai3::CoordinateSpace space : {ai3::CoordinateSpace::local, ai3::CoordinateSpace::parent,
                                       ai3::CoordinateSpace::world, ai3::CoordinateSpace::view})
        viewport.set_reference_space(space);
    CHECK(state.document_revision() == revision);
    CHECK(session.history().current_state_id() == history);
    CHECK_FALSE(session.dirty());
}
