# ADR 0008: Document materials and linear authored color

## Status

Accepted.

## Decision

Materials are independent ordered Scene Document entities with stable, monotonic `MaterialId` allocation and
deterministic default naming. They may remain unassigned. Only renderable semantic payloads carry optional
material references; currently that is `SpherePrimitive`. Cameras and lights do not have material assignments.
A sphere retains an independent fallback color while assigned, and object deletion never deletes a material.

Authored material, directional-light, and primitive fallback colors are finite `[0,1]` linear RGB.
Artist-facing pickers use the standard piecewise sRGB transfer function. Shaders consume linear values, perform
lighting in linear RGB, and explicitly encode final color to sRGB for the ordinary RGBA8 viewport target.
Normal mutations reject values outside `[0,1]`. V1 migration interprets legacy light colors as sRGB, clamps
them to the picker gamut, and converts them to linear.

Scene Document v2 stores materials and their allocator/name metadata, sphere assignments and fallback colors,
and linear light colors. V1 loading is a direct migration: spheres receive no assignment and the established
blue fallback. No generalized migration framework is introduced.

Unlit fallback, Lambert, and Phong are distinct linked concrete GLES3 programs. Material shading types map to
their program rather than selecting branches in one material-type uber-shader. A small GLES-only RAII helper
centralizes compilation, linking, diagnostics, uniform requirements, and deletion; it is not a shader system or
renderer/backend abstraction. The M14 ambient material color is an artist-authored contribution used directly
by the simplified Lambert/Phong equations, not a separate ambient or environment light.

## Consequences

Material edits use existing snapshot transactions and update every assigned sphere immediately. History and
persistence exclude compiled shaders, uniforms, GPU resources, and caches. Lambert and classic reflection-vector
Phong are the only shading models; textures, PBR, material deletion, browsers, shader graphs, and renderer
abstractions remain deferred.
