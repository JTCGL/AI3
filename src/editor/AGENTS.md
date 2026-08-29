# Editor Model Agent Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- Keep editor state independent of Dear ImGui and graphical platform APIs.
- Model only behavior required by the current editor milestone; do not introduce ECS, persistence,
  undo/redo, asset, or plugin abstractions.
- Keep dummy scene data explicit and deterministic so it can be tested without a display.
- Keep object identity, hierarchy, selection, renderable classification, and authoritative transforms here;
  renderers may consume this state but must not duplicate it.
- Authoritative positions are meters and orientations are `glm::quat`; display-unit and Euler-degree
  presentation must convert without replacing those canonical values.
