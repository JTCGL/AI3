#pragma once

#include "core/edit_history.h"

#include <filesystem>
#include <string>

namespace ai3
{
enum class DocumentTransition
{
    none,
    new_document,
    open_document,
    quit
};

enum class TransitionRequestResult
{
    proceed,
    needs_unsaved_resolution
};

enum class WorkspacePersistenceStatus
{
    not_attempted,
    succeeded,
    missing,
    failed
};

struct WorkspacePersistenceResult
{
    WorkspacePersistenceStatus status = WorkspacePersistenceStatus::not_attempted;
    std::string diagnostic;
};

struct DocumentSaveResult
{
    bool scene_saved = false;
    std::string scene_diagnostic;
    WorkspacePersistenceResult workspace;
};

struct DocumentOpenResult
{
    bool scene_opened = false;
    std::string scene_diagnostic;
    WorkspacePersistenceResult workspace;
};

class DocumentSession
{
    public:
    DocumentSession(Scene& scene, Workspace& workspace, EditHistory& history);

    bool dirty() const;
    const std::filesystem::path& document_path() const;
    DocumentRevision clean_revision() const;
    EditHistory& history();
    const EditHistory& history() const;
    DocumentTransition pending_transition() const;

    TransitionRequestResult request_transition(DocumentTransition transition);
    void cancel_pending_transition();
    DocumentTransition discard_and_take_pending_transition();
    DocumentTransition saved_and_take_pending_transition();
    void save_failed();

    void mark_saved();
    DocumentSaveResult save();
    DocumentSaveResult save_as(std::filesystem::path path);
    DocumentOpenResult open(std::filesystem::path path);
    void new_document();

    private:
    Scene& scene_;
    Workspace& workspace_;
    EditHistory& history_;
    std::filesystem::path document_path_;
    DocumentRevision clean_revision_ = 0;
    HistoryStateId clean_history_state_ = 0;
    DocumentTransition pending_transition_ = DocumentTransition::none;

    void mark_saved_as(std::filesystem::path path);
    void mark_opened(std::filesystem::path path);
    WorkspacePersistenceResult save_workspace();
};
} // namespace ai3
