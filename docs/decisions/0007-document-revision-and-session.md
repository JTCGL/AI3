# ADR 0007: Document revision and session ownership

## Status

Accepted.

## Decision

`EditorState` owns a monotonically increasing, display-independent revision for authoritative Scene Document
state. Every real mutation of a serialized value advances the revision; semantically unchanged assignments
and workspace/session interactions do not. Ordinary callers receive const scene-object access and perform
persistent changes through explicit `EditorState` mutation operations. The Scene Document codec retains the
narrow private reconstruction access required for transactional identity-preserving loads.

A display-independent `DocumentSession` owns the single active document's path association, clean-revision
baseline, dirty determination, and pending New/Open/Quit transition. It also coordinates document filesystem
operations. Dirty state is revision inequality, not a serialization/hash comparison; returning a value to its
saved value manually may therefore remain dirty until a successful save or document replacement establishes
a new baseline.

New, Open, Quit, and window-close requests use the same pending-transition policy. A clean request may proceed
immediately. A dirty request waits for Save, Discard, or Cancel. Saving permits the pending transition only
after successful Save or Save As; failed or cancelled saving cancels it. Open selection and loading begin only
after that resolution. There is always one active document, and no Close Document operation.

Reset Scene is an edit to the active document: it retains the path and advances the revision only when it
changes document state. New instead establishes a clean empty untitled document and clears the path.

## Consequences

UI code presents dialogs and applies approved transitions but is not authoritative for path, dirty, or pending
transition semantics. Selection, viewport state, display units, locale, panels, layout, diagnostics, console,
and renderer caches remain outside the document revision. Future undo/redo may replace the simple clean
revision boundary, but no command history or semantic snapshot comparison is introduced here.
