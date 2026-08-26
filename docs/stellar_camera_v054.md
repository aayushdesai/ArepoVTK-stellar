# Stellar camera v054

## Physical landmarks

The camera path consumes a strictly time-ordered table of snapshot number,
physical time, tracked center, angular-momentum axis, material radius, disk
radius, and polar extent. Landmark extraction remains a separate eta-only
analysis step with its own provenance. The camera planner never reads an AREPO
snapshot.

## Stabilization

- Axis signs are made continuous before filtering, eliminating apparent
  180-degree flips from the eigenvector/sign ambiguity.
- The azimuth reference is parallel-transported as the axis evolves. Camera
  roll therefore cannot jump when the axis crosses a world-coordinate basis
  threshold.
- Center tracking uses bounded speed and acceleration instead of directly
  copying a noisy center estimate.
- Axis rotation is limited by both a response time and an angular-rate cap.
- Material, disk, and outflow scales are tracked independently. Expansion and
  contraction use asymmetric hysteresis so transient diffuse cells cannot make
  the view breathe or jump outward. Disk shots stay tight while outflow shots
  retain the larger polar field of view.
- Every state update and pose is deterministic for a fixed landmark table and
  parameter set.

## Shot vocabulary

`stellar_camera_plan_v054` emits a production-ready camera table for one shot:

- `disk_edge`: resolves disk thickness and equatorial tails.
- `disk_oblique`: preserves disk morphology while showing vertical structure.
- `outflow_side`: frames both lobes against the disk.
- `outflow_axis`: looks down the polar axis to expose opening angle and
  collimation, using a transverse scale rather than the full polar length.
- `outflow_follow`: follows one lobe with a tighter scale instead of making the
  central remnant tiny.
- `orbit`: applies deterministic azimuthal motion around the smoothed axis.

The planner writes camera position, look-at point, up vector, screen half
extent, and all three filtered extents for every snapshot. The orbit phase is
relative to the first landmark time, so the requested initial azimuth is stable
across cropped time windows. Output creation uses an exclusive no-clobber open.

## Acceptance

Unit gates cover sign continuity, angular-rate and acceleration bounds, scale
hysteresis, roll continuity across basis thresholds, determinism, orthogonal
camera bases, shot-specific framing, and finite poses.
Movie acceptance then compares identical reconstruction/transfer settings with
static and moving paths so a camera improvement cannot be confused with a
physics or color change.
