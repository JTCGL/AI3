# Architecture Tooling

AI3 maintains a repository-owned clang-uml configuration and architecture
snapshot pipeline.

The generated artifacts are build products. They are not authoritative source
files and should not normally be committed.

## Entry point

From the repository root, run:

    ./scripts/generate-architecture.sh

The generator selects a host profile automatically:

- `termux` uses the `termux-clang-debug` compilation database and applies the
  Clang resource-directory workaround required by the Termux clang-uml build.
- `linux` uses the `linux-gcc-debug` compilation database and does not apply
  the Termux workaround.

The profile may be selected explicitly:

    AI3_ARCH_PROFILE=termux ./scripts/generate-architecture.sh
    AI3_ARCH_PROFILE=linux ./scripts/generate-architecture.sh

`CLANG_UML` may be set to override the clang-uml executable.

The generator passes the selected compilation-database directory to clang-uml
with `--compile-database`. The `compilation_database_dir` value in
`clang-uml.yml` is therefore only a neutral fallback for direct/manual use.

## Models

Each snapshot contains three complementary clang-uml models.

### Complete type model

`complete/ai3_architecture.*`

Describes generated C++ class/type relationships within the AI3 namespace.

This is a semantic type model. It is not a complete implementation dependency
graph and must not be interpreted as one.

### Package model

`packages/ai3_packages.*`

Describes directory/package dependencies. The raw model intentionally retains
dependencies on third-party packages discovered through the build.

### Include model

`includes/ai3_includes.*`

Describes concrete source/header include relationships. System headers are
excluded.

The analyzer uses this model to derive AI3's internal cross-layer include
dependencies.

## Analysis

`scripts/analyze-architecture.py` consumes all three JSON models and produces
`analysis/architecture-analysis.txt`.

The report distinguishes type relationships, package dependencies, concrete
include relationships, and derived internal layer dependencies.

No architectural pass/fail policy is currently encoded. Reported dependencies
are evidence for architectural review, not violations by themselves.

## Snapshot provenance

Generated ZIP archives use immutable names of the form:

`ai3-architecture-<UTC timestamp>-<short Git SHA>[-dirty].zip`

`dirty` indicates that the repository contained uncommitted changes when the
snapshot was generated.

Each archive contains `SUMMARY.txt` and `metadata.json` recording the Git
state, host profile, compilation database, tool versions, model statistics,
and snapshot identity.

## Generated output

The working output is placed under the sibling build tree:

`../build/AI3/architecture/`

On Termux, the completed ZIP is also copied to:

`~/storage/downloads/`

On native Linux, it is copied to `~/Downloads/` when that directory exists.

Generated UML, JSON, SVG, analysis reports, and snapshot ZIP files should
remain build artifacts rather than repository source.
