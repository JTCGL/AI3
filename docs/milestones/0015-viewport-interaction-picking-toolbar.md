# Milestone 15 — Viewport interaction modes, object picking, and minimal editor toolbar

## Goal

Establish explicit viewport Selection and Navigation modes, intuitive selection of rendered spheres, and a
minimal contextual editor toolbar without introducing transform tools or generalized interaction frameworks.

## Implemented scope

- `ViewportView` owns Selection/Navigation workspace state independently of its Orbit/Scene Camera source.
  Mode changes preserve document revision, history, dirty state, selection, source selection, and persistence.
- UI navigation input dispatches through `ViewportView`. Orbit drag/zoom retain their established behavior;
  Scene Camera navigation changes neither the camera nor the retained Orbit pose.
- Normalized top-left viewport coordinates and a resolved view/projection produce a bounded near-to-far world
  ray. Picking considers exactly enabled, visible Sphere primitives, transforms the ray into authoritative local
  space, intersects the semantic radius, and returns the nearest world-ray hit. Hierarchies, rotation, uniform,
  non-uniform, and reflected scales are supported without a world bounding-sphere approximation; singular
  candidates fail safely.
- Selection-mode left click selects the hit sphere or clears selection on a miss. Selection input never orbits.
  Navigation-mode drag/zoom never performs selection.
- A localized horizontal toolbar is a DPI-scaled main-viewport sidebar below the menu and above the dockspace.
  It presents mutually exclusive mode controls plus a minimal contextual region while leaving the existing
  view-source control in the Viewport panel.

## Deferred

Transform gizmos and Move/Rotate/Scale tools, snapping, multi-selection, highlighting, selection filters,
additional pickable primitives, GPU picking, collision infrastructure, new view/navigation types, scene-camera
manipulation, toolbar customization, generalized tool/toolbar/plugin frameworks, and Scene Document changes
remain outside this milestone.

## Verification

Run `bash scripts/check.sh`. Headless tests cover mode defaults and workspace exclusions, Orbit-only navigation,
unchanged Scene Cameras, center/off-center rays, Orbit and Scene Camera picking, translated and hierarchical
spheres, rotated/non-uniform/reflected transforms, nearest-hit ordering, visibility/enabled eligibility, misses,
projection clipping, and singular transforms. Physical runtime review on supported targets remains required.
