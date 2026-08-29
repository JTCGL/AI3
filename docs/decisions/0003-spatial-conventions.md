# ADR 0003: Canonical spatial conventions

## Status

Accepted.

## Decision

AI3 world space is right-handed with +X right, +Y forward, and +Z up. Scene and editor model lengths are
stored in meters. Metric display units (mm, cm, m, and km) convert only at presentation boundaries and the
default display unit is meters.

Authoritative orientations are stored as `glm::quat`. Human-facing rotations are Euler angles in degrees
using intrinsic XYZ order: rotate about local X, then the resulting local Y, then the resulting local Z.
Positive angles follow the right-hand rule. Central scene helpers own conversions between these forms;
radians remain an implementation detail at GLM boundaries.

## Consequences

Camera, transform, procedural-geometry, and future editor math must use the Z-up convention. Importers and
exporters must convert foreign axes and units at their boundaries. Changing display units never rescales
stored transforms. Quaternion-to-Euler conversion may return any equivalent representation; continuity and
cached-Euler editing behavior remain deferred until transform-editor work requires them.
