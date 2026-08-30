# ADR 0006: Versioned Scene Documents and controlled identity reconstruction

## Status

Accepted.

## Decision

AI3 Scene Documents use UTF-8 JSON, the `.ai3scene` extension, the format identifier `ai3-scene`, and an
integer format version. Version 1 is a strict schema: required fields must be present, unsupported fields and
unknown semantic types are rejected, and no pre-v1 migration or speculative forward-compatibility mechanism
exists. Categories and subtypes use durable string names rather than C++ enum ordinals. Quaternions are stored
as normalized `[w, x, y, z]` arrays; transforms are authoritative local-to-parent TRS in meters. World
transforms, Euler presentation, derived geometry, renderer state, and editor workspace/session state are not
document data.

The display-independent editor target owns the codec and filesystem helpers and uses pinned nlohmann/json.
Loading parses and validates a complete candidate before replacing only the scene-owned object vector,
allocator, default-name counters, and selection. A narrowly friended codec may restore explicit IDs and
parents; normal editor operations still allocate through `EditorState` lifecycle APIs. Stored local hierarchy
data is reconstructed directly, never through the interactive world-pose-preserving reparent operation.

Validation rejects invalid identity, hierarchy, category/subtype combinations, local transforms, semantic
payloads, allocator state, and naming-counter metadata. Successful loading clears selection. The application
workflow separately associates the path, resets `ViewportView` to default Orbit, and invalidates renderer
geometry caches. A failed load changes none of those values.

SDL3 asynchronous native file dialogs implement Open and Save As at the graphical UI boundary. Their callback
only transfers a path or error through thread-safe lifetime-managed state; document I/O and model/UI mutation
run on the application thread.

## Consequences

Scene persistence remains headless-testable and does not add SDL, Dear ImGui, EGL/GLES, or renderer
dependencies to the editor model. Version 1 files preserve exact nonzero IDs, object order, hierarchy, local
state, current semantic payloads, the next-ID allocator, and subtype default-name counters across process
lifetimes. Schema evolution must be an explicit later decision. Workspace persistence, dirty tracking,
autosave, recovery, recent files, and multi-document behavior remain separate future concerns.
