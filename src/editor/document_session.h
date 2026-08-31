#pragma once

#include "editor/editor_history.h"

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

class DocumentSession
{
    public:
    explicit DocumentSession(EditorState& state);

    bool dirty() const;
    const std::filesystem::path& document_path() const;
    DocumentRevision clean_revision() const;
    EditorHistory& history();
    const EditorHistory& history() const;
    DocumentTransition pending_transition() const;

    TransitionRequestResult request_transition(DocumentTransition transition);
    void cancel_pending_transition();
    DocumentTransition discard_and_take_pending_transition();
    DocumentTransition saved_and_take_pending_transition();
    void save_failed();

    void mark_saved();
    void mark_saved_as(std::filesystem::path path);
    void mark_opened(std::filesystem::path path);
    bool save(std::string* error = nullptr);
    bool save_as(std::filesystem::path path, std::string* error = nullptr);
    bool open(std::filesystem::path path, std::string* error = nullptr);
    void new_document();
    bool reset_scene();

    private:
    EditorState& state_;
    EditorHistory history_;
    std::filesystem::path document_path_;
    DocumentRevision clean_revision_ = 0;
    HistoryStateId clean_history_state_ = 0;
    DocumentTransition pending_transition_ = DocumentTransition::none;
};
} // namespace ai3
