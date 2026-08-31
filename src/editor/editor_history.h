#pragma once

#include "editor/editor_state.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace ai3
{
using HistoryStateId = std::uint64_t;

class EditorHistory
{
    public:
    explicit EditorHistory(EditorState& state);

    bool begin_transaction();
    bool commit_transaction();
    bool cancel_transaction();
    bool transaction_active() const;
    bool has_uncommitted_changes() const;

    bool can_undo() const;
    bool can_redo() const;
    bool undo();
    bool redo();

    HistoryStateId current_state_id() const;
    void rebaseline();

    private:
    struct Snapshot
    {
        std::vector<SceneObject> objects;
        ObjectId next_object_id = 1;
        std::map<EditorState::SubtypeKey, std::uint64_t> default_name_counts;
    };
    struct Entry
    {
        Snapshot before;
        Snapshot after;
        HistoryStateId before_id = 0;
        HistoryStateId after_id = 0;
    };

    Snapshot capture() const;
    static bool snapshots_equal(const Snapshot& left, const Snapshot& right);
    void restore(const Snapshot& snapshot);
    HistoryStateId allocate_state_id();

    EditorState& state_;
    std::vector<Entry> entries_;
    std::size_t position_ = 0;
    HistoryStateId baseline_id_ = 1;
    HistoryStateId next_state_id_ = 2;
    bool transaction_active_ = false;
    Snapshot transaction_before_;
};
} // namespace ai3
