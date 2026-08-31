# Milestone 12 — Document dirty state and unsaved-change protection

## Goal

Distinguish authoritative Scene Document mutation from workspace/session interaction and provide safe,
display-independent lifecycle semantics for AI3's single active document.

## Approved scope

- Centralize ordinary serialized-state changes in explicit `EditorState` mutation operations and retain only
  the codec's narrow reconstruction seam.
- Track a monotonically increasing document revision, advancing once for each real authoritative mutation and
  never for no-op assignments or workspace-only changes.
- Add a headless `DocumentSession` that owns the associated path, clean revision, dirty state, document I/O,
  and pending destructive transition.
- Add localized New and Save/Discard/Cancel protection for New, Open, Quit, and SDL window-close requests.
- Preserve transactional Open behavior and show the Open chooser only after dirty-state resolution.
- Present a restrained dirty marker in the application title while keeping UI identity independent of the
  displayed title.

## Invariants and semantics

There is exactly one active Scene Document and no Close Document command. A new empty untitled document starts
clean. Successful Open, Save, and Save As establish the current revision as clean; Open and Save As update the
path. New creates a clean empty untitled document and clears the path while leaving unrelated workspace state
alone.

Reset Scene is not New. It retains the associated path and becomes dirty when content, allocator state, or
naming counters actually change. Resetting an already canonical empty scene is a no-op.

Serialized object identity, ordering, hierarchy, local transforms, flags, names, semantic payloads, allocator,
and naming counters are authoritative document state. Selection, viewport/Orbit state, display length unit,
locale, panels/layout, console/diagnostics, renderer caches, and other presentation state do not affect the
revision. Stored lengths remain meters.

## Unsaved-change state machine

A clean New/Open/Quit/window-close request proceeds directly. A dirty request records its semantic transition
and waits for Save, Discard, or Cancel. Save uses the associated path or invokes Save As; only success releases
the transition. A failed save or cancelled Save As cancels the transition and retains the current document.
Discard releases it without saving, and Cancel abandons it. Dirty Open does not show its file chooser until the
current document is resolved; failed replacement loading leaves the current document and session association
unchanged.

## Explicit exclusions

Undo/redo and command history, autosave/recovery, recent files, multiple documents/tabs, Close Document,
import/export, assets, format changes or migration, configurable document units, workspace persistence,
renderer ownership changes, headless GLES, custom file-browser UI, ECS, event bus, plugins, and a generic
command framework are excluded.

## Automated verification

Headless doctest coverage verifies revision changes and no-ops across lifecycle, fields, hierarchy, transforms,
and semantic payloads; workspace-only non-dirty behavior; session clean/path/reset semantics; pending transition
resolution; transactional invalid Open; and existing persistence, identity, hierarchy, naming, and transform
regressions. Run `bash scripts/check.sh`.

## Manual runtime verification

1. Launch with a clean empty untitled document; edit every supported object field and observe the dirty marker.
2. Save and Save As, then change only selection, Orbit/view source, display units, locale, panels/layout, and
   diagnostics; confirm the document stays clean.
3. Reset a saved non-empty scene and confirm its path remains associated while it becomes dirty.
4. Exercise clean and dirty New and Open, including every Save/Discard/Cancel branch and a cancelled Save As.
5. Confirm a failed save or invalid Open retains the current valid document and blocks replacement.
6. Exercise File → Quit and the SDL window close control while dirty; Cancel must keep AI3 running.
7. Confirm no Close Document action exists.

## Completion evidence

- Termux ARM64 runtime verification passed.
- T5600 Linux x86-64 runtime verification passed.
- GitHub Actions passed for the final reviewed implementation.
