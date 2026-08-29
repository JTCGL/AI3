# UI Layer Instructions

These instructions extend src/AGENTS.md and the repository root instructions.

- Keep editor-facing state and drawing separate from ImGui lifecycle/backend ownership.
- All user-facing labels, menu entries, panel titles, empty-state messages, and inspector field labels must come from the localization system once it is available.
- Do not use displayed English text as an internal identity. Docking/window identity must remain stable across locale changes; use hidden/stable ImGui IDs where needed so translating a title does not destroy persisted docking state.
- Treat localized strings as UTF-8 and do not truncate by byte count when code-point-safe behavior is required.
- Keep DPI scaling coherent across font size, style paddings, spacing, minimum sizes, and other visual metrics.
- Do not scatter magic pixel constants through the UI. Route deliberate fixed design metrics through a centralized scale-aware UI metric/style layer when one exists.
- Preserve normal ImGui .ini docking persistence across language changes and DPI changes.
- Keep Dear ImGui developer/debug windows available separately from localized editor-facing panels.
