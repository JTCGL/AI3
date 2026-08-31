#include <doctest/doctest.h>

#include "editor/document_session.h"

#include <chrono>
#include <filesystem>
#include <fstream>

namespace
{
template <typename Edit> void edit(ai3::DocumentSession& session, Edit&& operation)
{
    REQUIRE(session.history().begin_transaction());
    operation();
    session.history().commit_transaction();
}
} // namespace

TEST_CASE("document session tracks clean baselines and paths")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    CHECK_FALSE(session.dirty());
    CHECK(session.document_path().empty());

    edit(session, [&] { state.create_sphere("Sphere"); });
    CHECK(session.dirty());
    session.mark_saved_as("first.ai3scene");
    CHECK_FALSE(session.dirty());
    CHECK(session.document_path() == "first.ai3scene");

    edit(session, [&] { state.rename_object(1, "Changed"); });
    CHECK(session.dirty());
    session.mark_saved();
    CHECK_FALSE(session.dirty());
    edit(session, [&] { state.set_object_visible(1, false); });
    CHECK(session.dirty());

    session.mark_opened("opened.ai3scene");
    CHECK_FALSE(session.dirty());
    CHECK(session.document_path() == "opened.ai3scene");
    session.new_document();
    CHECK(state.objects().empty());
    CHECK(session.document_path().empty());
    CHECK_FALSE(session.dirty());
}

TEST_CASE("saved history checkpoints drive dirty state through undo redo and branching")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    edit(session, [&] { state.create_sphere("Sphere"); });
    session.mark_saved_as("saved.ai3scene");
    CHECK_FALSE(session.dirty());
    edit(session, [&] { state.rename_object(1, "Edited"); });
    CHECK(session.dirty());
    REQUIRE(session.history().undo());
    CHECK_FALSE(session.dirty());
    REQUIRE(session.history().redo());
    CHECK(session.dirty());
    REQUIRE(session.history().undo());
    edit(session, [&] { state.set_object_visible(1, false); });
    CHECK(session.dirty());
    CHECK_FALSE(session.history().can_redo());
    session.mark_saved();
    CHECK_FALSE(session.dirty());
}

TEST_CASE("session file open is transactional and successful open is clean")
{
    const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path valid = std::filesystem::temp_directory_path() /
                                        ("ai3-session-" + std::to_string(unique) + ".ai3scene");
    const std::filesystem::path invalid = valid.string() + ".invalid";
    ai3::EditorState source;
    source.create_sphere("Loaded");
    ai3::DocumentSession source_session(source);
    REQUIRE(source_session.save_as(valid));

    ai3::EditorState state;
    const ai3::ObjectId existing = state.create_object(ai3::CreateObject{"Existing"});
    ai3::DocumentSession session(state);
    session.mark_saved_as("original.ai3scene");
    edit(session, [&] { state.rename_object(existing, "Dirty existing"); });
    REQUIRE(session.history().can_undo());
    const auto path_before_failure = session.document_path();
    const bool dirty_before_failure = session.dirty();
    {
        std::ofstream stream(invalid);
        stream << "invalid";
    }
    CHECK_FALSE(session.open(invalid));
    REQUIRE(state.find_object(existing) != nullptr);
    CHECK(session.document_path() == path_before_failure);
    CHECK(session.dirty() == dirty_before_failure);
    CHECK(session.history().can_undo());

    REQUIRE(session.open(valid));
    CHECK(state.objects().size() == 1);
    CHECK(state.objects()[0].name == "Loaded 1");
    CHECK(session.document_path() == valid);
    CHECK_FALSE(session.dirty());
    CHECK_FALSE(session.history().can_undo());
    CHECK_FALSE(session.history().can_redo());
    std::filesystem::remove(valid);
    std::filesystem::remove(invalid);
}

TEST_CASE("new document clears history and establishes an untitled clean baseline")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    edit(session, [&] { state.create_sphere("Sphere"); });
    session.mark_saved_as("old.ai3scene");
    edit(session, [&] { state.rename_object(1, "Changed"); });
    REQUIRE(session.history().can_undo());
    session.new_document();
    CHECK(state.objects().empty());
    CHECK(session.document_path().empty());
    CHECK_FALSE(session.dirty());
    CHECK_FALSE(session.history().can_undo());
    CHECK_FALSE(session.history().can_redo());
}

TEST_CASE("workspace-only changes remain outside history and dirty state")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    state.set_panel_visible(ai3::EditorPanel::console, false);
    state.add_console_message("diagnostic");
    state.request_layout_reset();
    CHECK_FALSE(session.dirty());
    CHECK_FALSE(session.history().can_undo());
}

TEST_CASE("reset scene retains association and only dirties for a real change")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    session.mark_saved_as("scene.ai3scene");
    CHECK_FALSE(session.reset_scene());
    CHECK_FALSE(session.dirty());
    edit(session, [&] { state.create_object(ai3::CreateObject{"Object"}); });
    session.mark_saved();
    REQUIRE(session.reset_scene());
    CHECK(session.dirty());
    CHECK(session.document_path() == "scene.ai3scene");
    REQUIRE(session.history().undo());
    CHECK_FALSE(session.dirty());
    CHECK(session.document_path() == "scene.ai3scene");
    REQUIRE(session.history().redo());
    CHECK(session.dirty());
}

TEST_CASE("pending destructive transitions enforce save discard and cancel")
{
    ai3::EditorState state;
    ai3::DocumentSession session(state);
    for (ai3::DocumentTransition transition :
         {ai3::DocumentTransition::new_document, ai3::DocumentTransition::open_document,
          ai3::DocumentTransition::quit})
        CHECK(session.request_transition(transition) == ai3::TransitionRequestResult::proceed);

    edit(session, [&] { state.create_object(ai3::CreateObject{"Dirty"}); });
    CHECK(session.request_transition(ai3::DocumentTransition::open_document) ==
          ai3::TransitionRequestResult::needs_unsaved_resolution);
    CHECK(session.pending_transition() == ai3::DocumentTransition::open_document);
    session.cancel_pending_transition();
    CHECK(session.pending_transition() == ai3::DocumentTransition::none);

    session.request_transition(ai3::DocumentTransition::new_document);
    CHECK(session.discard_and_take_pending_transition() == ai3::DocumentTransition::new_document);
    CHECK(session.dirty());

    session.request_transition(ai3::DocumentTransition::quit);
    session.save_failed();
    CHECK(session.pending_transition() == ai3::DocumentTransition::none);
    CHECK(session.dirty());

    session.request_transition(ai3::DocumentTransition::open_document);
    CHECK(session.saved_and_take_pending_transition() == ai3::DocumentTransition::open_document);
    CHECK_FALSE(session.dirty());
}
