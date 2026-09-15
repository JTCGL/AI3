# Milestone 20: Core architecture contract

## Goal

Establish AI3 Core as the display-independent architectural center and define the ownership and dependency
contract that later milestones will implement incrementally.

## Approved scope

- Add ADR 0009 defining Core, Scene/Scene Document, Workspace, frontend presentation, semantic operations,
  undo/redo, viewport/tools, persistence, rendering, platform, and Application boundaries.
- Record the M21-M27 migration program without presenting it as implemented.
- Reconcile current-truth documentation and agent guidance with the approved direction and known temporary
  violations.
- Keep this milestone strictly documentation and architecture-contract work.

## Explicit exclusions

No C++ or CMake behavior changes, source moves, class renames, `ai3::Core` façade, empty `ai3_core` target,
dependency changes, or migration work from M21 onward are in scope. The program also does not design Vulkan,
DirectX, math abstraction, ECS, plugins, generic command/message/event buses, scripting, networking/IPC,
multiple documents or viewports, new transforms, material or mesh redesign, Scene Document format changes, or
a generic renderer abstraction.

## Acceptance criteria

- Core terminology and ownership are defined, including allowed and forbidden dependency directions.
- Current implementation and approved target architecture are clearly distinguished.
- ADR 0009 explicitly supersedes ADR 0007's conflicting transaction-boundary ownership without rewriting the
  historical ADR; its other revision, dirty-state, history, checkpoint, and transition semantics remain valid.
- The M21-M27 migration is recorded and overlapping roadmap entries are reconciled.
- Repository agent instructions no longer encourage dependencies designated for removal and do not pretend the
  post-M27 source layout exists.
- No runtime, source, or CMake behavior changes occur.
- `bash scripts/check.sh` passes. No graphical runtime verification is required for a documentation-only change.
