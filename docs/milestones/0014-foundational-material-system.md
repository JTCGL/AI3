# Milestone 14 — Foundational material system

## Goal

Add a narrow complete material vertical slice: reusable document materials, Lambert and classic Phong shading,
sphere assignment/fallback appearance, live transactional editing, and versioned persistence with a coherent
linear-lighting color contract.

## Implemented scope

- Independent ordered materials use stable monotonic IDs and names and may exist unassigned.
- Spheres retain optional assignments and authoritative linear fallback colors; cameras and lights do not.
- Unlit fallback, Lambert, and reflection-vector Phong use three distinct linked GLES3 programs through a
  small concrete RAII compile/link helper. Resolved Orbit and scene camera views supply world-space eye position.
- Lighting is linear RGB and fragment output explicitly applies standard sRGB encoding for the RGBA8 target.
- A floating non-docking Material Editor selects among all document materials with a top-of-window dropdown,
  creates new independent materials, edits one workspace-active material independently of object selection,
  and assigns it to a selected sphere. The sphere inspector reports assignment and edits fallback color.
- The ambient parameter is an artist-authored contribution in the simplified M14 Lambert/Phong equations;
  there is no ambient-light or environment-light object.
- Material, fallback, and light edits use existing history gesture boundaries; all new UI text is localized.
- Scene Document v2 persists complete material state and linear colors. V1 loading assigns no material,
  supplies the established blue fallback, and converts legacy light color from sRGB to linear.

## Deferred

Material deletion/duplication, browsers, previews, textures, multiple slots, PBR, arbitrary shaders, renderer
abstractions, and unrelated editor interactions remain out of scope.

## Verification

Run `bash scripts/check.sh`. Headless tests cover material authority, assignment, validation/no-ops, history,
color transfers, resolved eye positions, v2 persistence, dangling references, and v1 migration. Physical runtime
review remains required on supported targets.
