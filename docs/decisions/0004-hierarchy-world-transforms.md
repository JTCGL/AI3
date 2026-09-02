# ADR 0004: Authoritative hierarchy and world transforms

## Status

Accepted.

## Decision

Scene-object transforms store local-to-parent TRS. Root local transforms equal world transforms, while child
world matrices are recursively composed as parent world multiplied by child local. `EditorState` owns the
single category-agnostic hierarchy resolver and the only operation that changes an existing object's parent.
All object categories may parent all other categories. Only missing parents, self-parenting, descendants as
parents, and cycles are rejected.

Reparenting is transactional and preserves the old world matrix. The desired new local matrix is the inverse
new-parent world matrix multiplied by the old child world matrix, or simply the old world matrix when moving
to the root. GLM decomposes that affine matrix to position, normalized quaternion, and scale. AI3 accepts the
result only when all values are finite, the parent is invertible, and recomposing the candidate TRS matches
each desired matrix element within an absolute-plus-relative tolerance. Matrix elements are compared
independently, so translation magnitude cannot loosen validation of the linear transform.

Reflected transforms and negative scale components use the same reconstruction test and are accepted only
when GLM returns a finite TRS that reproduces the requested matrix. Numerically singular parent matrices
(absolute determinant at or below `1e-6`) are treated as non-invertible and rejected.

Because arbitrary rotated non-uniform scales can produce shear, not every affine local matrix fits AI3's TRS
storage. Such reparent operations are rejected without changing either hierarchy or transform. AI3 does not
silently discard skew and does not add an arbitrary-affine transform representation.

Local, Parent, World, and View are separate reference-space concepts for future tools. Their bases do not
change local-to-parent storage semantics. Local and Parent bases use resolved scene orientations, World uses
canonical scene axes, and View uses the independently supplied viewport/editor camera basis.

Axis translation uses a narrower authoritative operation than reparenting. A desired world pivot is converted
through the inverse authoritative parent world matrix to a new local position; a root uses the desired position
directly. The operation preserves local orientation, local scale, and hierarchy exactly, and rejects non-finite
input or a parent determinant at or below the existing `1e-6` invertibility threshold without mutation. It does
not decompose a desired world matrix or establish a general world-transform setter.

## Consequences

Render, camera, light, picking, gizmo, and animation consumers must use the authoritative world resolver
instead of walking parent chains. Primitive geometry remains derived only from semantic primitive parameters;
local/ancestor transform and hierarchy changes do not regenerate it. Directional-light direction and scene
camera forward direction derive from resolved world orientation, while the editor orbit camera remains outside
the scene hierarchy.
