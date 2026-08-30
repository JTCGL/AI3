# Script Agent Instructions

These instructions extend the repository root AGENTS.md.

- Scripts must be idempotent where practical.
- bootstrap.sh manages project prerequisites, not the entire operating system.
- Never perform an unconditional full-system upgrade.
- Detect Termux separately from conventional Debian/Ubuntu Linux.
- Use Clang on Termux and GCC on conventional Linux.
- Do not install SDL3 or Dear ImGui globally; CMake owns those source dependencies.
- Keep scripts thin wrappers around canonical CMake presets and project tooling.
- Preserve the clang-format 21.x formatting policy and the CMake 3.25 minimum with explicit version checks;
  never fall back silently to an incompatible host-tool version.
- Quote shell variables and fail on errors.
