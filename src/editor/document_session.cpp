#include "editor/document_session.h"
#include "editor/scene_document.h"

#include <utility>

namespace ai3
{
DocumentSession::DocumentSession(EditorState& state)
    : state_(state), history_(state), clean_revision_(state.document_revision()),
      clean_history_state_(history_.current_state_id())
{
}

bool DocumentSession::dirty() const
{
    return history_.current_state_id() != clean_history_state_ ||
           history_.has_uncommitted_changes();
}
const std::filesystem::path& DocumentSession::document_path() const { return document_path_; }
DocumentRevision DocumentSession::clean_revision() const { return clean_revision_; }
EditorHistory& DocumentSession::history() { return history_; }
const EditorHistory& DocumentSession::history() const { return history_; }
DocumentTransition DocumentSession::pending_transition() const { return pending_transition_; }

TransitionRequestResult DocumentSession::request_transition(DocumentTransition transition)
{
    if (!dirty())
        return TransitionRequestResult::proceed;
    pending_transition_ = transition;
    return TransitionRequestResult::needs_unsaved_resolution;
}

void DocumentSession::cancel_pending_transition()
{
    pending_transition_ = DocumentTransition::none;
}

DocumentTransition DocumentSession::discard_and_take_pending_transition()
{
    return std::exchange(pending_transition_, DocumentTransition::none);
}

DocumentTransition DocumentSession::saved_and_take_pending_transition()
{
    mark_saved();
    return std::exchange(pending_transition_, DocumentTransition::none);
}

void DocumentSession::save_failed() { cancel_pending_transition(); }

void DocumentSession::mark_saved()
{
    clean_revision_ = state_.document_revision();
    clean_history_state_ = history_.current_state_id();
}

void DocumentSession::mark_saved_as(std::filesystem::path path)
{
    document_path_ = std::move(path);
    mark_saved();
}

void DocumentSession::mark_opened(std::filesystem::path path)
{
    document_path_ = std::move(path);
    history_.rebaseline();
    mark_saved();
}

bool DocumentSession::save(std::string* error)
{
    if (document_path_.empty())
    {
        if (error != nullptr)
            *error = "Scene Document has no associated path";
        return false;
    }
    if (!save_scene_document_file(state_, document_path_, error))
        return false;
    mark_saved();
    return true;
}

bool DocumentSession::save_as(std::filesystem::path path, std::string* error)
{
    if (!save_scene_document_file(state_, path, error))
        return false;
    mark_saved_as(std::move(path));
    return true;
}

bool DocumentSession::open(std::filesystem::path path, std::string* error)
{
    if (!load_scene_document_file(path, state_, error))
        return false;
    mark_opened(std::move(path));
    return true;
}

void DocumentSession::new_document()
{
    state_.reset_scene();
    history_.rebaseline();
    document_path_.clear();
    mark_saved();
}

bool DocumentSession::reset_scene()
{
    if (!history_.begin_transaction())
        return false;
    const bool changed = state_.reset_scene();
    history_.commit_transaction();
    return changed;
}
} // namespace ai3
