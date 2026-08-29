# GitHub Agent Instructions

These instructions extend the repository root AGENTS.md.

- GitHub Actions is an independent clean-environment verifier.
- CI must use the same CMake presets and verification path used locally wherever practical.
- The initial CI target is Linux x86-64 with GCC.
- CI passing on Ubuntu does not prove Termux runtime compatibility.
- Keep workflows small and readable; move project logic into scripts or CMake rather than embedding it in YAML.
- Run CI for pull requests and pushes to main.
