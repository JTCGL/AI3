# Editor Model Agent Instructions

These instructions extend `src/AGENTS.md` and the repository root instructions.

- Keep editor state independent of Dear ImGui and graphical platform APIs.
- Model only behavior required by the current editor milestone; do not introduce ECS, persistence,
  undo/redo, asset, or plugin abstractions.
- Keep dummy scene data explicit and deterministic so it can be tested without a display.
