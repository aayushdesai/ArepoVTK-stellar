# Cinematic transition windows v059

`stellar_camera_director_v059` keeps physical landmark tracking separate from
artistic shot direction and replaces v056's outgoing, end-anchored transitions
with explicit incoming transition windows.

## Shot schema

```text
# schema=stellar_cinematic_shots_v059
# name start_snapshot end_snapshot mode subject azimuth_deg elevation_deg framing_margin orbit_degrees orbit_period_seconds lobe_sign transition_end_snapshot easing
```

The first shot must use `-` for `transition_end_snapshot`. A later shot may
also use `-` to request an explicit cut. Otherwise its incoming transition
starts at `start_snapshot` and reaches the new shot exactly at
`transition_end_snapshot`. The transition end must be a landmark row inside
that shot and later than the start. Interpolation uses physical simulation
time and the selected `linear`, `smoothstep`, or `smootherstep` easing.

During a transition, both shot poses are evaluated against the same filtered
physical center, axis, disk extent, and outflow extent. Only artistic camera
direction and framing are blended. The previous shot's finite orbit is held at
its final phase; the incoming shot begins at its initial phase.

## Motion gate

Production path generation defaults to these perceptual limits:

```text
--max-roll-deg-per-frame 0.25
--max-zoom-percent-per-frame 1.0
```

Dry runs always write no-clobber diagnostics, a manifest, and an SVG preview.
The diagnostics include per-frame roll and zoom displacement. The manifest
records the limits, maxima, worst snapshots, and pass state. A production run
that exceeds either limit writes the review products but does not write the
camera path and exits with status 2.

This gate makes an impossible instruction visible. A camera cannot hold one
framing through snapshot N and also arrive at a substantially different
framing at N+1 without a cut. The shot author must instead start the incoming
transition earlier, finish it later, reduce the framing change, or explicitly
accept a cut after reviewing the diagnostics.

The emitted camera table remains `stellar_camera_path_v055`, so the existing
native path consumer and direction-manifest provenance remain unchanged.
