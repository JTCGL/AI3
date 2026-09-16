#pragma once

#include "core/scene.h"
#include "core/workspace.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace ai3
{
using HistoryStateId = std::uint64_t;

class EditHistory
{
    public:
    EditHistory(Scene& scene, Workspace& workspace);
    EditHistory(const EditHistory&) = delete;
    EditHistory& operator=(const EditHistory&) = delete;

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
        Scene scene;
        std::map<ObjectId, BoundsDisplayState> object_lifecycle_bounds;
    };
    struct Entry
    {
        Snapshot before;
        Snapshot after;
        HistoryStateId before_id = 0;
        HistoryStateId after_id = 0;
        std::map<ObjectId, BoundsDisplayState> deleted_object_workspace;
    };

    Snapshot capture() const;
    static bool snapshots_equal(const Snapshot& left, const Snapshot& right);
    void restore(const Snapshot& snapshot);
    HistoryStateId allocate_state_id();

    Scene& scene_;
    Workspace& workspace_;
    std::vector<Entry> entries_;
    std::size_t position_ = 0;
    HistoryStateId baseline_id_ = 1;
    HistoryStateId next_state_id_ = 2;
    bool transaction_active_ = false;
    Snapshot transaction_before_;
};
} // namespace ai3
