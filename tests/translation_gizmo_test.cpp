#include <doctest/doctest.h>

#include "core/document_session.h"
#include "editor/editor_state.h"
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
    ai3::ViewportView viewport(state.workspace());
    const ai3::ResolvedViewportView view = viewport.resolve(state.scene(), 1.0F);
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
    ai3::ViewportView viewport(state.workspace());
    const ai3::ResolvedViewportView view = viewport.resolve(state.scene(), 1.0F);
    const glm::mat3 basis{1.0F};
    const auto near_gizmo =
        ai3::project_translation_gizmo({}, basis, view, {800.0F, 600.0F}, 72.0F);
    const glm::vec3 farther = glm::normalize(view.eye_position) * -4.0F;
    const auto far_gizmo =
        ai3::project_translation_gizmo(farther, basis, view, {800.0F, 600.0F}, 72.0F);
    REQUIRE(near_gizmo.has_value());
    REQUIRE(far_gizmo.has_value());
    for (std::size_t index = 0; index < 3; ++index)
    {
        REQUIRE(near_gizmo->endpoints[index].has_value());
        REQUIRE(far_gizmo->endpoints[index].has_value());
        CHECK(glm::length(*near_gizmo->endpoints[index] - near_gizmo->pivot) ==
              doctest::Approx(72.0F));
        CHECK(glm::length(*far_gizmo->endpoints[index] - far_gizmo->pivot) ==
              doctest::Approx(72.0F));
    }
    CHECK(ai3::pick_translation_axis(
              near_gizmo->pivot +
                  glm::normalize(*near_gizmo->endpoints[0] - near_gizmo->pivot) * 30.0F,
              *near_gizmo, 8.0F) == ai3::TranslationAxis::x);

    const glm::vec3 view_aligned = glm::transpose(glm::mat3{view.view})[2];
    const glm::mat3 collapsed_basis{view_aligned, view_aligned, view_aligned};
    const auto collapsed =
        ai3::project_translation_gizmo({}, collapsed_basis, view, {800.0F, 600.0F}, 72.0F);
    REQUIRE(collapsed.has_value());
    CHECK_FALSE(collapsed->endpoints[0].has_value());
    CHECK(ai3::pick_translation_axis(collapsed->pivot, *collapsed, 8.0F) ==
          ai3::TranslationAxis::none);
    CHECK_FALSE(
        ai3::project_translation_gizmo({}, basis, view, {800.0F, 600.0F}, 0.0F).has_value());
    CHECK_FALSE(ai3::project_translation_gizmo(
                    {}, basis, view, {std::numeric_limits<float>::quiet_NaN(), 600.0F}, 72.0F)
                    .has_value());
}

TEST_CASE("frozen viewport geometry rejects material coordinate frame changes")
{
    CHECK(ai3::viewport_geometry_matches({100.0F, 200.0F}, {800.0F, 600.0F}, {100.25F, 199.75F},
                                         {800.25F, 600.25F}));
    CHECK_FALSE(ai3::viewport_geometry_matches({100.0F, 200.0F}, {800.0F, 600.0F}, {101.0F, 200.0F},
                                               {800.0F, 600.0F}));
    CHECK_FALSE(ai3::viewport_geometry_matches({100.0F, 200.0F}, {800.0F, 600.0F}, {100.0F, 200.0F},
                                               {799.0F, 600.0F}));
}

TEST_CASE("translation gesture is one history transaction with checkpoint semantics")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state.scene(), state.workspace(), state.history());
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
    ai3::DocumentSession session(state.scene(), state.workspace(), state.history());
    ai3::ViewportView viewport(state.workspace());
    const ai3::DocumentRevision revision = state.document_revision();
    const ai3::HistoryStateId history = session.history().current_state_id();
    viewport.set_transform_tool(ai3::ViewportTransformTool::translation);
    for (ai3::CoordinateSpace space : {ai3::CoordinateSpace::local, ai3::CoordinateSpace::parent,
                                       ai3::CoordinateSpace::world, ai3::CoordinateSpace::view})
    {
        viewport.set_reference_space(space);
        CHECK(viewport.reference_space() == space);
    }
    CHECK(state.document_revision() == revision);
    CHECK(session.history().current_state_id() == history);
    CHECK_FALSE(session.dirty());
}

TEST_CASE("translation controller acquires updates and commits one authored edit")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    const ai3::ObjectId object = operations.create_object(ai3::CreateObject{"Object"});
    REQUIRE(operations.select(object));
    history.rebaseline();
    ai3::ViewportView viewport(workspace);
    const ai3::ResolvedViewportView view = viewport.resolve(scene, 4.0F / 3.0F);
    const glm::vec2 size{800.0F, 600.0F};
    const auto projected = ai3::project_translation_gizmo({}, glm::mat3{1.0F}, view, size, 72.0F);
    REQUIRE(projected);
    REQUIRE(projected->endpoints[0]);
    const glm::vec2 axis = glm::normalize(*projected->endpoints[0] - projected->pivot);
    const glm::vec2 pointer = projected->pivot + axis * 36.0F;

    ai3::TranslationInteractionController controller(scene, workspace, operations);
    REQUIRE(controller.acquire(pointer, {100.0F, 200.0F}, size, view, 72.0F, 10.0F));
    REQUIRE(controller.gesture());
    CHECK(controller.gesture()->selected_axis == ai3::TranslationAxis::x);
    REQUIRE(controller.update(pointer + axis * 20.0F + glm::vec2{100.0F, 200.0F}, {100.0F, 200.0F},
                              size));
    const glm::vec3 first = scene.world_position(object);
    REQUIRE(controller.update(pointer + axis * 40.0F + glm::vec2{100.0F, 200.0F}, {100.0F, 200.0F},
                              size));
    const glm::vec3 final = scene.world_position(object);
    CHECK(final != first);
    REQUIRE(controller.commit());
    REQUIRE(history.undo());
    check_vec3(scene.world_position(object), {});
    REQUIRE(history.redo());
    check_vec3(scene.world_position(object), final);
}

TEST_CASE("translation controller cancellation and abandonment restore exact authored state")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    const ai3::ObjectId object = operations.create_object(ai3::CreateObject{"Object"});
    REQUIRE(operations.select(object));
    history.rebaseline();
    ai3::ViewportView viewport(workspace);
    const ai3::ResolvedViewportView view = viewport.resolve(scene, 1.0F);
    const glm::vec2 size{600.0F, 600.0F};
    const auto projected = ai3::project_translation_gizmo({}, glm::mat3{1.0F}, view, size, 72.0F);
    REQUIRE(projected);
    REQUIRE(projected->endpoints[0]);
    const glm::vec2 axis = glm::normalize(*projected->endpoints[0] - projected->pivot);
    const glm::vec2 pointer = projected->pivot + axis * 36.0F;

    ai3::TranslationInteractionController controller(scene, workspace, operations);
    REQUIRE(controller.acquire(pointer, {}, size, view, 72.0F, 10.0F));
    REQUIRE(controller.update(pointer + axis * 30.0F, {}, size));
    REQUIRE(controller.cancel());
    check_vec3(scene.world_position(object), {});
    CHECK_FALSE(history.can_undo());

    REQUIRE(controller.acquire(pointer, {}, size, view, 72.0F, 10.0F));
    REQUIRE(controller.update(pointer + axis * 30.0F, {}, size));
    CHECK_FALSE(controller.update(pointer + axis * 30.0F, {1.0F, 0.0F}, size));
    check_vec3(scene.world_position(object), {});
    CHECK_FALSE(controller.active());

    REQUIRE(controller.acquire(pointer, {}, size, view, 72.0F, 10.0F));
    REQUIRE(operations.delete_object(object));
    CHECK_FALSE(controller.update(pointer, {}, size));
    REQUIRE(scene.find_object(object));
    check_vec3(scene.world_position(object), {});
    CHECK_FALSE(history.transaction_active());
}

TEST_CASE("translation controller freezes reference basis and handles no-op and failure safely")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    ai3::Transform parent_transform;
    parent_transform.scale = {1.0F, 0.0F, 1.0F};
    const ai3::ObjectId parent =
        operations.create_object(ai3::CreateObject{"Parent", ai3::no_object, parent_transform});
    const ai3::ObjectId child =
        operations.create_object(ai3::CreateObject{"Child", parent, ai3::Transform{}});
    REQUIRE(operations.select(child));
    history.rebaseline();
    workspace.viewport().reference_space = ai3::CoordinateSpace::world;
    ai3::ViewportView viewport(workspace);
    const ai3::ResolvedViewportView view = viewport.resolve(scene, 1.0F);
    const glm::vec2 size{600.0F, 600.0F};
    const glm::vec3 pivot = scene.world_position(child);
    const auto projected =
        ai3::project_translation_gizmo(pivot, glm::mat3{1.0F}, view, size, 72.0F);
    REQUIRE(projected);
    REQUIRE(projected->endpoints[0]);
    const glm::vec2 axis = glm::normalize(*projected->endpoints[0] - projected->pivot);
    const glm::vec2 pointer = projected->pivot + axis * 36.0F;

    ai3::TranslationInteractionController controller(scene, workspace, operations);
    REQUIRE(controller.acquire(pointer, {}, size, view, 72.0F, 10.0F));
    const glm::mat3 frozen = controller.gesture()->frozen_basis;
    workspace.viewport().reference_space = ai3::CoordinateSpace::view;
    CHECK(controller.gesture()->frozen_basis == frozen);
    CHECK_FALSE(controller.update(pointer + axis * 30.0F, {}, size));
    CHECK_FALSE(controller.active());
    check_vec3(scene.world_position(child), pivot);
    CHECK_FALSE(history.transaction_active());

    workspace.viewport().reference_space = ai3::CoordinateSpace::world;
    REQUIRE(controller.acquire(pointer, {}, size, view, 72.0F, 10.0F));
    CHECK_FALSE(controller.commit());
    CHECK_FALSE(history.can_undo());
    CHECK_FALSE(controller.acquire({-1000.0F, -1000.0F}, {}, size, view, 72.0F, 10.0F));
}

TEST_CASE("translation controller acquires Local Parent World and View bases")
{
    ai3::Scene scene;
    ai3::Workspace workspace;
    ai3::EditHistory history(scene, workspace);
    ai3::EditOperations operations(scene, workspace, history);
    ai3::Transform parent_transform;
    parent_transform.orientation = ai3::orientation_from_euler_degrees({15.0F, 25.0F, 35.0F});
    const ai3::ObjectId parent =
        operations.create_object(ai3::CreateObject{"Parent", ai3::no_object, parent_transform});
    ai3::Transform child_transform;
    child_transform.orientation = ai3::orientation_from_euler_degrees({-10.0F, 20.0F, 5.0F});
    const ai3::ObjectId child =
        operations.create_object(ai3::CreateObject{"Child", parent, child_transform});
    REQUIRE(operations.select(child));
    ai3::ViewportView viewport(workspace);
    const ai3::ResolvedViewportView view = viewport.resolve(scene, 4.0F / 3.0F);
    const glm::vec2 size{800.0F, 600.0F};

    for (const ai3::CoordinateSpace space :
         {ai3::CoordinateSpace::local, ai3::CoordinateSpace::parent, ai3::CoordinateSpace::world,
          ai3::CoordinateSpace::view})
    {
        workspace.viewport().reference_space = space;
        const glm::mat3 expected = ai3::coordinate_space_basis(scene, child, space, view.view);
        const auto projected = ai3::project_translation_gizmo(scene.world_position(child), expected,
                                                              view, size, 72.0F);
        REQUIRE(projected);
        std::size_t index = 0;
        while (index < 3 && !projected->endpoints[index])
            ++index;
        REQUIRE(index < 3);
        const glm::vec2 pointer =
            projected->pivot + (*projected->endpoints[index] - projected->pivot) * 0.5F;
        ai3::TranslationInteractionController controller(scene, workspace, operations);
        REQUIRE(controller.acquire(pointer, {}, size, view, 72.0F, 10.0F));
        CHECK(controller.gesture()->frozen_basis == expected);
        REQUIRE(controller.cancel());
    }
}
