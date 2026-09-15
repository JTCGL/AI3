# ADR 0009: Core architecture boundaries

## Status

Accepted.

## Context

AI3's display-independent behavior has grown across targets named `editor` and `scene`, while `EditorState`
currently combines authored content, workspace state, and some frontend presentation state. `EditorUi` also
coordinates document workflows, interaction transactions, and renderer lifetime. These interfaces are current
implementation truth, but their names and ownership do not provide a durable dependency contract.

## Decision

AI3 Core is the architectural center of AI3. "Editor" and "Application" are not names for this central layer.
Application is reserved for executable composition and lifecycle. A frontend presents state and translates
external or user interaction into Core operations; Dear ImGui is one frontend. SDL is a platform adapter, and
GLES is the current concrete rendering backend.

Core is an architectural layer, not a requirement for one giant class. It may coordinate smaller Scene,
Workspace, History, Viewport/Tool, document/session, and other components, and it may eventually expose an
`ai3::Core` façade. This decision does not create that façade. Core must be usable without Dear ImGui, SDL,
GLES/OpenGL/EGL, GLSL, a window, a graphics context, or a display server. GLM remains the canonical math library;
math-type abstraction is deferred.

Core Scene owns authored AI3 content and its invariants: objects and IDs, hierarchy, transforms, primitive
semantic parameters, materials, lights, cameras, Scene Document revision, and persisted naming-counter
semantics. Scene state contains no frontend state. A Scene Document is the persistent representation and
lifecycle of authored Scene content, not a competing scene model. The current Scene Document format and behavior
must remain unchanged throughout this migration unless a separately approved change requires otherwise.

Core Workspace owns state meaningful while operating on content but not authored into a Scene Document. This
includes selection, bounds-display state, active material/editor selection where applicable, display units,
and appropriate viewport/editor preferences. Core ownership does not make Workspace changes Scene Document
changes. Selection belongs to Workspace, not Scene or ImGui. History may restore relevant workspace state with
a semantic operation such as deletion without making that state part of the Scene Document.

Frontend presentation state remains outside Core, including ImGui panel visibility and docking layout, native
dialog state, ImGui layout-reset requests, and GUI Console-window presentation or scrollback. Core may report
operation results and diagnostics for a frontend to present. No generic event bus is established.

Core owns typed semantic editing operations. Frontends express intent and must not construct AI3 behavior by
manually combining lower-level mutation, selection, history, and synchronization calls. The intended direction
is an operation such as `core.create_sphere(...)`, without requiring a generic command or message bus. Concrete
typed C++ operations will be introduced only as responsibilities migrate.

Core owns undo/redo semantics and semantic transaction behavior. A frontend owns concrete input lifecycle facts
such as pointer activation, movement, release, Escape, and widget activation/deactivation. It may communicate
begin/update/commit/cancel intent for a continuous semantic operation, but it is not the authority that builds
the internal history semantics. This supersedes only ADR 0007's assignment of transaction-boundary choice to UI
code. ADR 0007's revision, dirty-state, history, checkpoint, and document-transition semantics remain accepted.

Display-independent viewport state belongs to Core. `ViewportView` is an AI3 concept, not an ImGui window.
Display-independent navigation, picking and spatial queries, reference-space semantics, transform-tool behavior,
and continuous-transform semantics belong inward of the frontend boundary as appropriate. Concrete mouse,
button, and modifier interpretation remains a frontend/platform responsibility. No generalized tool framework
is established.

Persistence operates on appropriate Scene Document and Workspace representations and must not require an
aggregate frontend/editor object merely because current code does. Filesystem/document encoding may continue
to use nlohmann/json; no JSON abstraction is required. File selection and dialog presentation belong to
frontend/platform integration, while opening or saving a selected path is a Core/document operation.

The renderer will consume only renderer-relevant Core/Scene representations or queries rather than aggregate
editor/frontend state. GPU resources, GLES handles, shaders, GLSL, framebuffers, and render caches remain below
the renderer/backend boundary. Renderer invalidation and synchronization are not frontend responsibilities.
The final extraction representation and cache-invalidation mechanism remain deferred. AI3 retains one concrete
GLES renderer; this decision introduces neither a generic renderer abstraction nor Vulkan-oriented design.

SDL remains outside Core and owns concrete platform responsibilities including windowing, events, display
scale, GLES context creation, buffer swapping, and currently used native integration. Application may eventually
compose Core, SDL platform integration, an ImGui frontend, and GLES rendering.

The target dependency direction is:

```text
Frontend / CLI / tests / future interfaces -> Core -> domain, persistence, and render-facing seams
Renderer implementation -> renderer-relevant Core representation
GLES -> concrete renderer implementation
```

Accordingly, Core must not depend on ImGui, SDL, GLES/OpenGL/EGL, or GLSL; Scene must not depend on frontend
state; the renderer must not depend on history, session, panel, or dialog semantics; and persistence must not
depend on ImGui or SDL dialogs.

## Migration

The contract will be implemented incrementally. Existing classes and CMake targets remain authoritative until
each responsibility is deliberately migrated. `EditorState` may temporarily act as a compatibility façade but
must not be renamed to `Core`, because renaming would conceal its mixed ownership rather than correct it. Every
migration pull request must leave `main` buildable, tested, and behaviorally equivalent unless a separately
approved behavior change says otherwise. Temporary violations must be documented and removed in their assigned
milestones, not opportunistically fixed outside approved scope.

## Consequences

Future architecture work has one name and an enforceable inward dependency direction while preserving current
behavior during migration. Core semantics remain usable by graphical frontends, CLI code, tests, and future
interfaces without a graphics or display stack. This decision does not change source, CMake targets, runtime
behavior, persistence formats, renderer/history/viewport behavior, or introduce implementation for ECS,
plugins, scripting, networking, IPC, multiple documents/viewports, new transforms, material/mesh redesign, or
future rendering backends.
