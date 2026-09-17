#include "core/document_session.h"

#include "core/scene_document.h"
#include "core/workspace_document.h"

#include <system_error>
#include <utility>

namespace ai3
{
DocumentSession::DocumentSession(Scene& scene, Workspace& workspace, EditHistory& history)
    : scene_(scene), workspace_(workspace), history_(history),
      clean_revision_(scene.document_revision()), clean_history_state_(history.current_state_id())
{
}

bool DocumentSession::dirty() const
{
    return history_.current_state_id() != clean_history_state_ ||
           history_.has_uncommitted_changes();
}
const std::filesystem::path& DocumentSession::document_path() const { return document_path_; }
DocumentRevision DocumentSession::clean_revision() const { return clean_revision_; }
EditHistory& DocumentSession::history() { return history_; }
const EditHistory& DocumentSession::history() const { return history_; }
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
    clean_revision_ = scene_.document_revision();
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

DocumentSaveResult DocumentSession::save()
{
    DocumentSaveResult result;
    if (document_path_.empty())
    {
        result.scene_diagnostic = "Scene Document has no associated path";
        return result;
    }
    if (!save_scene_document_file(scene_, document_path_, &result.scene_diagnostic))
        return result;
    mark_saved();
    result.scene_saved = true;
    result.workspace = save_workspace();
    return result;
}

DocumentSaveResult DocumentSession::save_as(std::filesystem::path path)
{
    DocumentSaveResult result;
    if (!save_scene_document_file(scene_, path, &result.scene_diagnostic))
        return result;
    mark_saved_as(std::move(path));
    result.scene_saved = true;
    result.workspace = save_workspace();
    return result;
}

DocumentOpenResult DocumentSession::open(std::filesystem::path path)
{
    DocumentOpenResult result;
    if (!load_scene_document_file(path, scene_, &result.scene_diagnostic))
        return result;

    result.scene_opened = true;
    workspace_.transition_document();
    mark_opened(std::move(path));

    const std::filesystem::path workspace_path = workspace_path_for_scene(document_path_);
    std::error_code status_error;
    const bool sidecar_exists = std::filesystem::exists(workspace_path, status_error);
    if (status_error)
    {
        result.workspace.status = WorkspacePersistenceStatus::failed;
        result.workspace.diagnostic = status_error.message();
        return result;
    }
    if (!sidecar_exists)
    {
        result.workspace.status = WorkspacePersistenceStatus::missing;
        return result;
    }

    WorkspaceDocument document;
    if (!load_workspace_file(workspace_path, document, &result.workspace.diagnostic))
    {
        result.workspace.status = WorkspacePersistenceStatus::failed;
        return result;
    }
    for (auto item = document.objects.begin(); item != document.objects.end();)
        if (scene_.find_object(item->first) == nullptr)
            item = document.objects.erase(item);
        else
            ++item;
    workspace_.replace_bounds_display(std::move(document.objects));
    result.workspace.status = WorkspacePersistenceStatus::succeeded;
    return result;
}

WorkspacePersistenceResult DocumentSession::save_workspace()
{
    WorkspacePersistenceResult result;
    const WorkspaceDocument document{workspace_.bounds_display_states()};
    if (save_workspace_file(document, workspace_path_for_scene(document_path_), &result.diagnostic))
        result.status = WorkspacePersistenceStatus::succeeded;
    else
        result.status = WorkspacePersistenceStatus::failed;
    return result;
}

void DocumentSession::new_document()
{
    scene_.reset_scene();
    workspace_.transition_document();
    history_.rebaseline();
    document_path_.clear();
    mark_saved();
}
} // namespace ai3
