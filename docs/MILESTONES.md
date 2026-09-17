# Completed Milestones

This concise ledger records what each completed milestone established. It is historical context, not the
source of current architectural truth; see [`PROJECT.md`](PROJECT.md) for the current system.

## Milestone 1 — Initial SDL3/GLES3/ImGui application shell

- Merged: [PR #3](https://github.com/JTCGL/AI3/pull/3), `a0ecf24a0db2827287f94ff5ac70b6edbd9d7775`
- Established the first graphical executable with pinned SDL3 and Dear ImGui, an SDL-managed GLES3 context,
  docking, diagnostics, and the Termux:X11 build path.

## Milestone 2 — Harden and compartmentalize the application shell

- Merged: [PR #5](https://github.com/JTCGL/AI3/pull/5), `2fddae8cc724ed2a5d1dbddbe10ff0e899960af5`
- Split run-loop, platform, and UI ownership; added command-line tests, bounded real-stack smoke mode,
  formatting checks, and the shared local/CI verification path.

## Milestone 3 — Visible editor shell

- Merged: [PR #7](https://github.com/JTCGL/AI3/pull/7), `4fbe3f45e5a307ecd4440f01ef530d87dd8ac739`
- Added display-independent editor state and the docked Scene Graph, Viewport placeholder, Object Inspector,
  and Console with persistent layout behavior.

## Milestone 4 — Localization and DPI-aware UI foundation

- Merged: [PR #9](https://github.com/JTCGL/AI3/pull/9), `3d17d2b6c237b72e498d1fe33123542d72805059`
- Added external UTF-8 locales with runtime switching/fallback, stable localized ImGui identities, and
  SDL-driven runtime font/style scaling.

## Milestone 5 — Basic interactive scene

- Merged: [PR #11](https://github.com/JTCGL/AI3/pull/11), `1596a42490ea771642d7427562ad2bbad663f2a5`
- Added display-independent scene math and orbit camera, a concrete GLES3 offscreen viewport renderer, a
  rendered primitive driven by editor transform state, and viewport orbit/zoom input.

## Milestone 6 — Spatial, headless, lifecycle, and sphere foundation

- Merged through [PR #13](https://github.com/JTCGL/AI3/pull/13),
  `922a0905452e9c2ac07712f47f3e266d2849b916`; [PR #15](https://github.com/JTCGL/AI3/pull/15),
  `ecc3fb6338103021dfbffa01963747eada35fe4e`; [PR #17](https://github.com/JTCGL/AI3/pull/17),
  `84b5fd395155ea985a4a7091d946a4954713db97`; [PR #19](https://github.com/JTCGL/AI3/pull/19),
  `c9208d926e74f1c115aa249ffb3e67dbf0f4e453`; and [PR #21](https://github.com/JTCGL/AI3/pull/21),
  `7470ceaf4c30cb596707968876d1352ba629792b`.
- Adopted GLM; established right-handed Z-up, meters, quaternion/Euler, and display-unit conventions; added
  the graphics-free build/test lane; and replaced the provisional scene with runtime object lifecycle,
  semantic spheres, derived/cached sphere geometry, monotonic naming/identity, and Reset Scene behavior.

## Milestone 7 — Basic camera and light scene objects

- Merged: [PR #22](https://github.com/JTCGL/AI3/pull/22), `b2c9a2a093de89cf002b641b8b6a2925b3a1aaeb`
- Added tagged scene-object categories/subtypes with perspective cameras and directional lights, their
  lifecycle and inspector semantics, and directional lighting of viewport spheres.

## Milestone 8 — Hierarchy, world transforms, and coordinate spaces

- Merged: [PR #23](https://github.com/JTCGL/AI3/pull/23), `f69ce435b6e5f5f656094f86f08092cfe4a0f487`
- Made `EditorState` the category-independent hierarchy/world-transform authority, added transactional
  world-pose-preserving reparenting with strict TRS validation, and established Local/Parent/World/View bases.

## Milestone 9 — Viewport and view architecture foundation

- Merged: [PR #24](https://github.com/JTCGL/AI3/pull/24), `083c6f75d9c019c1628ac499f4834c051c51c680`
- Moved persistent viewport view state and resolution outside Dear ImGui, added viewport-specific Orbit or
  Perspective Camera selection, and changed the concrete renderer to consume resolved view/projection values.

## Milestone 10 — Scene Graph hierarchy manipulation UI

- Merged: [PR #25](https://github.com/JTCGL/AI3/pull/25), `db05487316500b8d6590e51905ef8bb9c5a6a7be`
- Added category-independent Scene Graph drag/drop parenting and root unparenting through the authoritative
  reparent operation, with visible transactional failure reporting. Corrected deletion semantics so deleting
  an object deletes only that object; direct children survive as scene roots with preserved world-space poses.

## Milestone 11 — Scene Document persistence foundation

- Final reviewed implementation head for [PR #27](https://github.com/JTCGL/AI3/pull/27):
  `4d334ae26c2aaaa41415ca943fb95903f49890e9`
- Added strict, versioned UTF-8 JSON Scene Documents with transactional headless serialization and filesystem
  persistence, exact scene identity and hierarchy reconstruction, and localized SDL-native Open, Save, and
  Save As workflows with defined document/workspace boundaries.

## Milestone 12 — Document dirty state and unsaved-change protection

- Final reviewed implementation head for [PR #30](https://github.com/JTCGL/AI3/pull/30):
  `9a68d7a3bb6b96b0e1bf82904c14a0baf5447d4a`
- Added authoritative document revisions and a display-independent single-document session, centralized
  serialized-state mutations, and localized Save/Discard/Cancel protection for New, Open, Quit, and window
  close. Runtime verification passed on Termux ARM64 and T5600 Linux x86-64, and GitHub Actions passed.

## Milestone 13 — Undo/Redo and editor transaction foundation

- Final reviewed implementation head for [PR #31](https://github.com/JTCGL/AI3/pull/31):
  `3a2f0b1c2da90a7f4eb14bab41b1fdfc1ff2ef59`
- Added display-independent snapshot-backed linear history with intentional transaction boundaries,
  checkpoint-based clean/dirty semantics, undoable discrete and continuous inspector edits, localized Undo/Redo
  access, and document-replacement history baselines. Runtime verification passed on Termux ARM64 and T5600
  Linux x86-64, and GitHub Actions passed.

## Milestone 14 — Foundational material system

- Final reviewed implementation head for [PR #33](https://github.com/JTCGL/AI3/pull/33):
  `9b66780393e82dc49bea7a6118c735dbb4bff7e0`
- Added multiple reusable document-owned materials with stable identity, sphere assignment and independent
  unlit fallback color, Lambert and classic reflection-vector Phong shading, and a floating Material Editor
  with active-material selection and creation. Established three distinct GLES3 programs through a small
  concrete program helper, view-dependent Phong from the resolved world-space eye position, linear-RGB storage
  with sRGB-facing controls and explicit output encoding, and strict Scene Document v2/v1 migration integrated
  with Undo/Redo, dirty state, persistence, and headless tests. Runtime verification passed on Termux ARM64 and
  T5600 Linux x86-64, and GitHub Actions passed.

## Milestone 15 — Viewport interaction modes, object picking, and minimal editor toolbar

- Merged: [PR #34](https://github.com/JTCGL/AI3/pull/34), `466a83fd722533027497db2fad90dddf888437d5`
- Added independent Selection and Navigation viewport interaction modes, source-aware Orbit navigation,
  display-independent inverse-local CPU sphere picking, and a localized DPI-aware editor toolbar above the
  dockspace. Selection, mode changes, and navigation remain workspace actions outside Scene Document revision,
  history, dirty state, and persistence. Headless tests cover mode ownership, navigation dispatch, ray
  construction, transformed-sphere picking, eligibility, nearest hits, clipping, and invalid transforms.
  Physical runtime review and GitHub Actions passed for the final reviewed implementation.

## Milestone 16 — Single-object axis translation gizmo

- Merged: [PR #35](https://github.com/JTCGL/AI3/pull/35), `cf2f70a368a0a9e74c0339a31168d68624836103`
- Added the initial localized DPI-aware X/Y/Z translation presentation for the selected object in Local,
  Parent, World, and View spaces, with handle-first Selection-mode ownership and unchanged Orbit-only
  Navigation behavior. Milestone 17 subsequently replaced its Dear ImGui overlay presentation with the
  authoritative GLES helper path.
- Added a narrow authoritative world-position mutation, frozen gesture-start closest-point/fallback-plane
  constraints, approximately constant apparent sizing, and one existing history transaction per drag.
  Headless coverage includes hierarchy and invertibility cases, constraint degeneracies, projection/hit math,
  history cancellation/Undo/Redo/checkpoints, and workspace-state exclusions. Physical runtime review passed
  on T5600 Linux x86-64; Termux runtime review was deferred under the normal single-platform acceptance policy.

## Milestone 17 — Cached object bounds, GLES helper rendering, and per-document workspace foundation

- Merged: [PR #38](https://github.com/JTCGL/AI3/pull/38), `4147033a5644261a102eed3c7eaf71b34871f9c4`
- Added cached object-local AABB/sphere data for spheres, atomic `.ai3workspace` sidecars for per-object bounds
  display state, and localized controls excluded from document revision and ordinary history edits. Legacy v1
  helper-rendering fields are accepted and ignored.
- Added shared display-independent helper geometry consumed by a GLES3 presentation whose biased bounds remain
  depth tested while gizmos render on top. Gizmos render in Selection and Navigation but remain
  interactive only in Selection; object hover feedback is Selection-only. Deletion Undo/Redo restores/removes
  the affected object's workspace switches without making workspace edits generally undoable. The GLES helper
  presentation passed physical runtime review on T5600 Linux x86-64, and GitHub Actions passed for the final
  reviewed implementation.

## Milestone 18 — Editor View navigation foundation

- Final reviewed implementation head for [PR #43](https://github.com/JTCGL/AI3/pull/43):
  `754880babffe7f1a010c688699a608ca0936f80c`
- Renamed the non-authored Orbit source to localized Editor View while retaining its independent pose and pivot
  across Scene Camera switching. Added display-independent MMB pan, Shift+MMB orbit, and wheel zoom in both
  retained modes, with acquisition-time operation freezing, outside-viewport continuation, input priority,
  invalid-input rejection, and complete Scene Camera/document/workspace invariants.
- Automated build, test, format, documentation, code review, and GitHub Actions evidence passed. Mouse-dependent
  physical verification was unavailable and explicitly deferred and accepted by the user; it was not reported as
  passed and may be performed later without reopening M18 unless a defect is found.

## Milestone 19 — Box primitive and derived mesh foundation

- Final reviewed implementation head for [PR #44](https://github.com/JTCGL/AI3/pull/44):
  `f583182d64ee30c5e80aa95e9aa17036f8789599`
- Added the centered Box primitive with independent width/length/height tessellation, hard face normals, planar
  UVs, exact cached bounds, exact local-space picking, materials/fallback color, hierarchy/history/editor UI,
  and strict Scene Document v3 persistence with genuine v1/v2 compatibility.
- Extracted the neutral derived triangle-mesh representation shared by Sphere and Box, including UVs, while
  explicitly deferring the future artist-editable/topological mesh model. Canonical checks and GitHub Actions
  passed, and physical runtime verification passed on Termux ARM64 under the normal single-platform acceptance
  policy while the T5600 was unavailable.

## Milestone 20 — Core architecture contract

- Established AI3 Core as the approved display-independent architectural center and defined durable Scene,
  Scene Document, Workspace, frontend, semantic-operation, history, viewport/tool, persistence, renderer,
  platform, Application, and dependency boundaries in ADR 0009.
- Recorded the incremental M21-M27 migration, identified current compatibility violations without changing
  implementation, and reconciled agent guidance so new work does not deepen dependencies slated for removal.
  M20 is documentation-only: no source, CMake, persistence-format, or runtime behavior changed.

## Milestone 21 — Core Scene extraction

- Created the display-independent `ai3_core` target and made Core `Scene` the sole owner of authored objects,
  materials, identity/allocation and naming state, transforms/hierarchy, semantic payloads, bounds, and revision.
- Reduced `EditorState` to a compatibility façade around one `Scene` plus retained Workspace/frontend state;
  moved Scene Document persistence physically into Core and preserved v1/v2 migration and version 3 output.
- Converted authored scene math, viewport-camera resolution, primitive picking, renderer input, and direct Core
  tests to `Scene`; retained history/session, Workspace helper, and continuous-operation seams for M22-M27.

## Milestone 22 — Core Workspace extraction

- Added the display-independent Core `Workspace` as the sole owner of selection, per-object bounds-display
  state, active material selection, and display length unit; `EditorState` now coordinates exactly one `Scene`
  and one `Workspace` while retaining frontend presentation compatibility state.
- Moved Workspace Document v1 persistence and display-length conversion into `ai3_core` without expanding the
  sidecar boundary or changing history/dirty semantics, narrowed helper geometry to `Scene`/`Workspace`, and
  removed the transitional `ai3_scene` dependency on `ai3_editor`.

## Milestone 23 — Core semantic operations and undo boundary

- Moved snapshot-backed undo/redo authority into Core `EditHistory`, directly over `Scene` and `Workspace`,
  while preserving exact authored restoration, checkpoints, revision behavior, and deleted-object bounds-state
  lifecycle handling without making ordinary Workspace changes undoable.
- Added typed Core `EditOperations` with self-transactional discrete edits and participation in existing
  continuous transactions; moved object lifecycle and Scene-dependent Workspace coordination out of
  `EditorState`, which now remains a compatibility/presentation façade.
- Routed frontend semantic mutations and `DocumentSession` checkpoints through the Core authorities, retained
  the M23 continuous ImGui/gizmo transaction lifecycle for M24, and added direct graphics-free Core editing
  coverage without changing persistence formats or renderer behavior.

## Milestone 24 — Continuous operations and viewport/tools boundary

- Added Core-owned move-only continuous-edit lifetime with grouped snapshot-history commit, exact cancellation,
  no-op suppression, and safe abandonment; ordinary ImGui continuous edits now retain that lifetime across
  frames while continuing to call typed `EditOperations`.
- Moved authoritative view source, scene-camera identity, Editor View state, interaction mode, translation tool,
  and reference space into Core Workspace. `ViewportView` now operates on that state from `ai3_scene` without
  duplicating authority.
- Added a display-independent concrete translation interaction controller owning acquisition, frozen gesture
  state, semantic world-position updates, commit/cancel, and continuous history lifetime. Preserved navigation,
  picking, helper geometry, renderer boundaries, snapshot history, and Workspace Document v1 persistence.

## Milestone 25 — Core document/session and persistence boundary

- Moved `DocumentSession` into `ai3_core` over non-owning Scene, Workspace, and EditHistory references while
  preserving history checkpoints, authored dirty state, destructive-transition policy, and the single-document
  New/Open/Save/Save As workflow.
- Added operation-specific Core persistence results that distinguish missing and failed Workspace sidecars while
  keeping Scene persistence authoritative, failed Open transactional, and failed Save As path-safe.
- Added the Workspace-owned New/Open transition policy and valid-object filtering for restored v1 bounds state;
  preserved viewport/editor preferences outside the unchanged Workspace Document v1 format and retained
  frontend-owned dialogs, Console presentation, and transitional renderer-cache invalidation.
