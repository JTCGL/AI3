# ADR 0007: Document revision and session ownership

## Status

Accepted.

## Decision

`EditorState` owns a monotonically increasing, display-independent revision for authoritative Scene Document
state. Every real mutation of a serialized value advances the revision; semantically unchanged assignments
and workspace/session interactions do not. Ordinary callers receive const scene-object access and perform
persistent changes through explicit `EditorState` mutation operations. The Scene Document codec retains the
narrow private reconstruction access required for transactional identity-preserving loads.

A focused display-independent `EditorHistory` owns linear history-state identity and transaction lifecycle.
Its initial representation is authoritative in-memory before/after snapshots containing object identity and
order, hierarchy, local transforms, flags, names, semantic payloads, the next-ID allocator, and default-name
counters. It excludes selection and all other workspace, session, derived, renderer, and GPU state. The public
transaction API does not expose this representation, allowing later delta records without changing callers.

`DocumentSession` owns the single active document's path association, saved history-state checkpoint, dirty
determination, and pending New/Open/Quit transition. It also coordinates document filesystem operations.
Dirty state is current history-state identity inequality with the saved checkpoint. The monotonic document
revision remains in force and is not decremented by Undo: authoritative restoration advances it once when the
restored state differs.

New, Open, Quit, and window-close requests use the same pending-transition policy. A clean request may proceed
immediately. A dirty request waits for Save, Discard, or Cancel. Saving permits the pending transition only
after successful Save or Save As; failed or cancelled saving cancels it. Open selection and loading begin only
after that resolution. There is always one active document, and no Close Document operation.

Reset Scene is one undoable edit to the active document: it retains the path and advances the revision only
when it changes document state. New and successful Open clear prior history and establish clean baselines;
failed Open preserves the current document, association, history, and checkpoint relationship.

## Consequences

UI code presents dialogs and chooses transaction boundaries but is not authoritative for history, path,
dirty, or pending-transition semantics. Every normal authoritative mutation uses an `EditorState` operation
inside a history transaction; boundaries model user intent, so repeated live mutations from one inspector or
future gizmo gesture commit as one operation. Cancel restores the transaction start and commits nothing;
unchanged commits create no entry. Selection, viewport state, display units, locale, panels, layout,
diagnostics, console, and renderer caches remain outside history and document revision.
