#!/usr/bin/env bash
set -euo pipefail

fail()
{
    printf 'ERROR: %s\n' "$*" >&2
    exit 1
}

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"
BUILD_ROOT="$(cd -- "$REPO_ROOT/../build/AI3" && pwd)"

CONFIG="$REPO_ROOT/clang-uml.yml"
OUTPUT_DIR="$BUILD_ROOT/architecture"

# ---------------------------------------------------------------------------
# Architecture-generation profile
#
# Selection precedence:
#   1. AI3_ARCH_PROFILE environment variable
#   2. automatic environment detection
#
# Profiles affect only host/toolchain integration. The UML model itself is
# shared through clang-uml.yml.
# ---------------------------------------------------------------------------

REQUESTED_PROFILE="${AI3_ARCH_PROFILE:-auto}"

case "$REQUESTED_PROFILE" in
    auto)
        if [[ "${PREFIX:-}" == *com.termux* ]]; then
            PROFILE="termux"
        else
            PROFILE="linux"
        fi
        ;;
    termux|linux)
        PROFILE="$REQUESTED_PROFILE"
        ;;
    *)
        fail "Unknown AI3_ARCH_PROFILE: $REQUESTED_PROFILE (expected auto, termux, or linux)"
        ;;
esac

EXTRA_CLANG_UML_ARGS=()
RESOURCE_DIR_WORKAROUND="disabled"
DOWNLOAD_DIR=""

case "$PROFILE" in
    termux)
        BUILD_PRESET="termux-clang-debug"
        DEFAULT_CLANG_UML="$HOME/Projects/clang-uml/build/src/clang-uml"
        DOWNLOAD_DIR="$HOME/storage/downloads"

        command -v clang >/dev/null 2>&1 ||
            fail "clang not found"

        CLANG_RESOURCE_DIR="$(clang -print-resource-dir)"

        [[ -d "$CLANG_RESOURCE_DIR" ]] ||
            fail "Clang resource directory does not exist: $CLANG_RESOURCE_DIR"

        [[ -f "$CLANG_RESOURCE_DIR/include/stddef.h" ]] ||
            fail "Clang resource header missing: $CLANG_RESOURCE_DIR/include/stddef.h"

        EXTRA_CLANG_UML_ARGS+=(
            "--add-compile-flag=-resource-dir=$CLANG_RESOURCE_DIR"
        )

        RESOURCE_DIR_WORKAROUND="enabled"
        ;;

    linux)
        BUILD_PRESET="linux-gcc-debug"

        if command -v clang-uml >/dev/null 2>&1; then
            DEFAULT_CLANG_UML="$(command -v clang-uml)"
        else
            DEFAULT_CLANG_UML="$HOME/Projects/clang-uml/build/src/clang-uml"
        fi

        if [[ -d "$HOME/Downloads" ]]; then
            DOWNLOAD_DIR="$HOME/Downloads"
        fi
        ;;
esac

CLANG_UML="${CLANG_UML:-$DEFAULT_CLANG_UML}"
COMPDB="$BUILD_ROOT/$BUILD_PRESET/compile_commands.json"

COMPLETE_DIR="$OUTPUT_DIR/complete"
PACKAGES_DIR="$OUTPUT_DIR/packages"
INCLUDES_DIR="$OUTPUT_DIR/includes"
ANALYSIS_DIR="$OUTPUT_DIR/analysis"

TYPE_PUML="$COMPLETE_DIR/ai3_architecture.puml"
TYPE_JSON="$COMPLETE_DIR/ai3_architecture.json"
TYPE_SVG="$COMPLETE_DIR/ai3_architecture.svg"

PACKAGE_PUML="$PACKAGES_DIR/ai3_packages.puml"
PACKAGE_JSON="$PACKAGES_DIR/ai3_packages.json"
PACKAGE_SVG="$PACKAGES_DIR/ai3_packages.svg"

INCLUDE_PUML="$INCLUDES_DIR/ai3_includes.puml"
INCLUDE_JSON="$INCLUDES_DIR/ai3_includes.json"
INCLUDE_SVG="$INCLUDES_DIR/ai3_includes.svg"

SUMMARY="$OUTPUT_DIR/SUMMARY.txt"
METADATA="$OUTPUT_DIR/metadata.json"
ANALYSIS="$ANALYSIS_DIR/architecture-analysis.txt"
ANALYZER="$REPO_ROOT/scripts/analyze-architecture.py"
ZIP=""

# ---------------------------------------------------------------------------
# Prerequisites
# ---------------------------------------------------------------------------

printf 'AI3 architecture generator\n'
printf '  Profile:     %s\n' "$PROFILE"
printf '  Source:      %s\n' "$REPO_ROOT"
printf '  Build root:  %s\n' "$BUILD_ROOT"
printf '  Build preset:%s\n' " $BUILD_PRESET"
printf '  Output:      %s\n' "$OUTPUT_DIR"
printf '  clang-uml:   %s\n' "$CLANG_UML"
printf '  resource-dir workaround: %s\n' "$RESOURCE_DIR_WORKAROUND"
printf '\n'

[[ -f "$CONFIG" ]] ||
    fail "Missing clang-uml config: $CONFIG"

[[ -f "$COMPDB" ]] ||
    fail "Missing compilation database: $COMPDB"

[[ -x "$CLANG_UML" ]] ||
    fail "clang-uml is not executable: $CLANG_UML"

[[ -f "$ANALYZER" ]] ||
    fail "Missing architecture analyzer: $ANALYZER"

for command_name in plantuml python zip git; do
    command -v "$command_name" >/dev/null 2>&1 ||
        fail "Required command not found: $command_name"
done

# ---------------------------------------------------------------------------
# Clean generated output
# ---------------------------------------------------------------------------

printf 'Cleaning architecture output...\n'
rm -rf "$OUTPUT_DIR"
mkdir -p \
    "$COMPLETE_DIR" \
    "$PACKAGES_DIR" \
    "$INCLUDES_DIR" \
    "$ANALYSIS_DIR"

# ---------------------------------------------------------------------------
# Generate clang-uml outputs
# ---------------------------------------------------------------------------

cd "$REPO_ROOT"

generate_model()
{
    local diagram="$1"
    local destination="$2"
    local puml="$3"
    local json="$4"

    printf 'Generating %s...\n' "$diagram"

    "$CLANG_UML" \
        --config "$CONFIG" \
        --compile-database "$(dirname "$COMPDB")" \
        --output-directory "$destination" \
        --diagram-name "$diagram" \
        --generator plantuml \
        --generator json \
        "${EXTRA_CLANG_UML_ARGS[@]}"

    [[ -s "$puml" ]] ||
        fail "PUML output was not generated: $puml"

    [[ -s "$json" ]] ||
        fail "JSON output was not generated: $json"
}

generate_model \
    ai3_architecture \
    "$COMPLETE_DIR" \
    "$TYPE_PUML" \
    "$TYPE_JSON"

generate_model \
    ai3_packages \
    "$PACKAGES_DIR" \
    "$PACKAGE_PUML" \
    "$PACKAGE_JSON"

generate_model \
    ai3_includes \
    "$INCLUDES_DIR" \
    "$INCLUDE_PUML" \
    "$INCLUDE_JSON"

# ---------------------------------------------------------------------------
# Validate JSON models
# ---------------------------------------------------------------------------

printf '\nValidating clang-uml JSON models...\n'

python - \
    "$TYPE_JSON" \
    "$PACKAGE_JSON" \
    "$INCLUDE_JSON" <<'PYVALIDATE'
import json
import pathlib
import sys

expected = {
    "ai3_architecture": "class",
    "ai3_packages": "package",
    "ai3_includes": "include",
}

for filename in sys.argv[1:]:
    path = pathlib.Path(filename)

    with path.open("r", encoding="utf-8") as f:
        data = json.load(f)

    required = {
        "diagram_type",
        "elements",
        "metadata",
        "name",
        "relationships",
    }

    missing = sorted(required - data.keys())
    if missing:
        raise SystemExit(
            f"{path}: missing required keys: " + ", ".join(missing)
        )

    if not isinstance(data["elements"], list):
        raise SystemExit(f"{path}: 'elements' is not a list")

    if not isinstance(data["relationships"], list):
        raise SystemExit(f"{path}: 'relationships' is not a list")

    name = data.get("name")
    diagram_type = data.get("diagram_type")

    if name not in expected:
        raise SystemExit(f"{path}: unexpected diagram name: {name!r}")

    if diagram_type != expected[name]:
        raise SystemExit(
            f"{path}: expected diagram type {expected[name]!r}, "
            f"got {diagram_type!r}"
        )

    print(
        f"  {name}: "
        f"{len(data['elements'])} top-level elements, "
        f"{len(data['relationships'])} relationships"
    )
PYVALIDATE

# ---------------------------------------------------------------------------
# Analyze architecture
# ---------------------------------------------------------------------------

printf '\nAnalyzing architecture...\n'

python "$ANALYZER" \
    "$TYPE_JSON" \
    "$PACKAGE_JSON" \
    "$INCLUDE_JSON" \
    "$ANALYSIS"

[[ -s "$ANALYSIS" ]] ||
    fail "Architecture analysis was not generated: $ANALYSIS"

# ---------------------------------------------------------------------------
# Render SVG
# ---------------------------------------------------------------------------

printf '\nRendering SVG diagrams...\n'

plantuml -tsvg "$TYPE_PUML"
plantuml -tsvg "$PACKAGE_PUML"
plantuml -tsvg "$INCLUDE_PUML"

for svg in \
    "$TYPE_SVG" \
    "$PACKAGE_SVG" \
    "$INCLUDE_SVG"
do
    [[ -s "$svg" ]] ||
        fail "SVG output was not generated: $svg"
done

# ---------------------------------------------------------------------------
# Provenance
# ---------------------------------------------------------------------------

printf 'Recording provenance...\n'

GIT_COMMIT="$(git -C "$REPO_ROOT" rev-parse HEAD)"
GIT_BRANCH="$(git -C "$REPO_ROOT" branch --show-current)"

GIT_STATUS="$(git -C "$REPO_ROOT" status --porcelain)"

if [[ -n "$GIT_STATUS" ]]; then
    GIT_STATE="dirty"
else
    GIT_STATE="clean"
fi

CLANG_UML_VERSION="$("$CLANG_UML" --version | head -n 1)"
PLANTUML_VERSION="$(plantuml -version | head -n 1)"
PYTHON_VERSION="$(python --version 2>&1)"
GENERATED_UTC="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
ARCHIVE_TIMESTAMP="$(date -u '+%Y%m%dT%H%M%SZ')"
GIT_SHORT_COMMIT="$(git -C "$REPO_ROOT" rev-parse --short=8 HEAD)"

if [[ "$GIT_STATE" == "dirty" ]]; then
    ARCHIVE_BASENAME="ai3-architecture-${ARCHIVE_TIMESTAMP}-${GIT_SHORT_COMMIT}-dirty.zip"
else
    ARCHIVE_BASENAME="ai3-architecture-${ARCHIVE_TIMESTAMP}-${GIT_SHORT_COMMIT}.zip"
fi

ZIP="$OUTPUT_DIR/$ARCHIVE_BASENAME"
SNAPSHOT_ID="${ARCHIVE_BASENAME%.zip}"

if command -v clang >/dev/null 2>&1; then
    CLANG_VERSION="$(clang --version | head -n 1)"
else
    CLANG_VERSION="not used/available"
fi

export PROFILE
export BUILD_PRESET
export RESOURCE_DIR_WORKAROUND
export GIT_COMMIT
export GIT_BRANCH
export GIT_STATE
export GIT_STATUS
export CLANG_VERSION
export CLANG_UML_VERSION
export PLANTUML_VERSION
export PYTHON_VERSION
export GENERATED_UTC
export SNAPSHOT_ID
export ARCHIVE_BASENAME
export COMPDB
export CLANG_UML

python - \
    "$TYPE_JSON" \
    "$PACKAGE_JSON" \
    "$INCLUDE_JSON" \
    "$METADATA" \
    "$SUMMARY" <<'PY'
import collections
import json
import os
import pathlib
import sys


def load(path):
    with pathlib.Path(path).open("r", encoding="utf-8") as f:
        return json.load(f)


def recursive_elements(items):
    for item in items:
        yield item
        for key in ("elements", "children"):
            children = item.get(key)
            if isinstance(children, list):
                yield from recursive_elements(children)


type_data = load(sys.argv[1])
package_data = load(sys.argv[2])
include_data = load(sys.argv[3])

metadata_path = pathlib.Path(sys.argv[4])
summary_path = pathlib.Path(sys.argv[5])

models = {
    "complete": type_data,
    "packages": package_data,
    "includes": include_data,
}

dirty_paths = os.environ.get("GIT_STATUS", "").splitlines()


def model_metadata(data):
    recursive = list(recursive_elements(data.get("elements", [])))
    relationships = data.get("relationships", [])

    return {
        "name": data.get("name"),
        "type": data.get("diagram_type"),
        "top_level_elements": len(data.get("elements", [])),
        "recursive_elements": len(recursive),
        "relationships": len(relationships),
        "element_types": dict(sorted(collections.Counter(
            element.get("type", "<unknown>")
            for element in recursive
        ).items())),
        "relationship_types": dict(sorted(collections.Counter(
            relationship.get("type", "<unknown>")
            for relationship in relationships
        ).items())),
        "clang_uml_metadata": data.get("metadata", {}),
    }


model_info = {
    name: model_metadata(data)
    for name, data in models.items()
}

metadata = {
    "snapshot": {
        "id": os.environ["SNAPSHOT_ID"],
        "archive": os.environ["ARCHIVE_BASENAME"],
        "format_version": 3,
    },
    "repository": "JTCGL/AI3",
    "git": {
        "commit": os.environ["GIT_COMMIT"],
        "branch": os.environ["GIT_BRANCH"],
        "working_tree": os.environ["GIT_STATE"],
        "status": dirty_paths,
    },
    "generated_utc": os.environ["GENERATED_UTC"],
    "environment": {
        "profile": os.environ["PROFILE"],
        "compilation_database_profile": os.environ["BUILD_PRESET"],
        "compilation_database": os.environ["COMPDB"],
        "clang_resource_dir_workaround":
            os.environ["RESOURCE_DIR_WORKAROUND"] == "enabled",
    },
    "tools": {
        "clang": os.environ["CLANG_VERSION"],
        "clang_uml": os.environ["CLANG_UML_VERSION"],
        "clang_uml_executable": os.environ["CLANG_UML"],
        "plantuml": os.environ["PLANTUML_VERSION"],
        "python": os.environ["PYTHON_VERSION"],
    },
    "models": model_info,
}

with metadata_path.open("w", encoding="utf-8") as f:
    json.dump(metadata, f, indent=2)
    f.write("\n")

lines = [
    "AI3 Architecture Snapshot",
    "=========================",
    "",
    "Snapshot",
    "--------",
    f"ID: {os.environ['SNAPSHOT_ID']}",
    f"Archive: {os.environ['ARCHIVE_BASENAME']}",
    "Format: v3 multimodel",
    "",
    "Source",
    "------",
    "Repository: JTCGL/AI3",
    f"Commit: {os.environ['GIT_COMMIT']}",
    f"Branch: {os.environ['GIT_BRANCH'] or '(detached HEAD)'}",
    f"Working tree: {os.environ['GIT_STATE'].upper()}",
    f"Generated UTC: {os.environ['GENERATED_UTC']}",
    "",
]

if dirty_paths:
    lines.extend([
        "Uncommitted paths",
        "-----------------",
        *dirty_paths,
        "",
    ])

lines.extend([
    "Environment",
    "-----------",
    f"Profile: {os.environ['PROFILE']}",
    f"Compilation database profile: {os.environ['BUILD_PRESET']}",
    f"Compilation database: {os.environ['COMPDB']}",
    "Termux Clang resource-dir workaround: "
        f"{os.environ['RESOURCE_DIR_WORKAROUND']}",
    "",
    "Tools",
    "-----",
    f"Clang: {os.environ['CLANG_VERSION']}",
    f"clang-uml: {os.environ['CLANG_UML_VERSION']}",
    f"clang-uml executable: {os.environ['CLANG_UML']}",
    f"PlantUML: {os.environ['PLANTUML_VERSION']}",
    f"Python: {os.environ['PYTHON_VERSION']}",
    "",
    "Models",
    "------",
])

for name in ("complete", "packages", "includes"):
    info = model_info[name]
    lines.extend([
        f"{name}:",
        f"  diagram: {info['name']}",
        f"  type: {info['type']}",
        f"  top-level elements: {info['top_level_elements']}",
        f"  recursive elements: {info['recursive_elements']}",
        f"  relationships: {info['relationships']}",
    ])

lines.extend([
    "",
    "Interpretation",
    "--------------",
    "complete/: C++ semantic/type relationships generated by clang-uml.",
    "packages/: directory/package dependency model, including external deps.",
    "includes/: concrete source/header include relationships.",
    "analysis/: derived multimodel architecture analysis.",
    "",
    "The class/type model is not a complete implementation dependency graph.",
    "Use the package and include models as complementary dependency evidence.",
    "",
    "Artifacts",
    "---------",
    "complete/ai3_architecture.json",
    "complete/ai3_architecture.puml",
    "complete/ai3_architecture.svg",
    "packages/ai3_packages.json",
    "packages/ai3_packages.puml",
    "packages/ai3_packages.svg",
    "includes/ai3_includes.json",
    "includes/ai3_includes.puml",
    "includes/ai3_includes.svg",
    "analysis/architecture-analysis.txt",
    "metadata.json",
    "SUMMARY.txt",
    "",
])

with summary_path.open("w", encoding="utf-8") as f:
    f.write("\n".join(lines))
PY

[[ -s "$METADATA" ]] ||
    fail "metadata.json was not generated"

[[ -s "$SUMMARY" ]] ||
    fail "SUMMARY.txt was not generated"

# ---------------------------------------------------------------------------
# Package
# ---------------------------------------------------------------------------

printf 'Packaging snapshot...\n'

rm -f "$ZIP"

(
    cd "$OUTPUT_DIR"
    zip -qr "$ARCHIVE_BASENAME" \
        SUMMARY.txt \
        metadata.json \
        complete \
        packages \
        includes \
        analysis
)

[[ -s "$ZIP" ]] ||
    fail "ZIP archive was not generated: $ZIP"

# ---------------------------------------------------------------------------
# Delivery copy
# ---------------------------------------------------------------------------

if [[ -n "$DOWNLOAD_DIR" && -d "$DOWNLOAD_DIR" ]]; then
    cp "$ZIP" "$DOWNLOAD_DIR/$ARCHIVE_BASENAME"
    DELIVERY="$DOWNLOAD_DIR/$ARCHIVE_BASENAME"
else
    DELIVERY="not copied (no profile delivery directory available)"
fi

# ---------------------------------------------------------------------------
# Result
# ---------------------------------------------------------------------------

printf '\nArchitecture snapshot complete.\n\n'
cat "$SUMMARY"

printf '\nFiles:\n'
ls -lh \
    "$TYPE_JSON" "$TYPE_PUML" "$TYPE_SVG" \
    "$PACKAGE_JSON" "$PACKAGE_PUML" "$PACKAGE_SVG" \
    "$INCLUDE_JSON" "$INCLUDE_PUML" "$INCLUDE_SVG" \
    "$ANALYSIS" \
    "$SUMMARY" \
    "$METADATA" \
    "$ZIP"

printf '\n============================================================\n'
printf 'DONE — upload this architecture snapshot to ChatGPT:\n\n'
printf '%s\n' "$DELIVERY"
printf '============================================================\n'
