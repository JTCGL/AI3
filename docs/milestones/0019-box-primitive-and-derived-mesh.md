# Milestone 19: Box primitive and derived mesh foundation

M19 adds a centered, validated Box primitive with six hard-normal tessellated faces, exact bounds and ray/AABB picking. Sphere and Box share a derived indexed triangle mesh containing position, normal, and UV coordinates; Sphere uses equirectangular UVs. Scene Documents advance to strict version 3 while loading v1/v2, and Box parameters/material fallback persist durably.

Future editable/topological meshes remain deferred; this mesh is only a flattened derived render form. Automated verification uses the repository check and focused tests; physical runtime review is separate.
