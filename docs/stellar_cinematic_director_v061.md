# Stellar cinematic director v061

Version v061 adds explicit anchor-scale binding and declared cinematic cuts. It
is additive: v056, v059, and v060 remain unchanged, while accepted v061 output
continues to use the native `stellar_camera_path_v055` table.

## Why cuts are explicit

For the white-dwarf merger landmarks, a close disk frame near snapshot 721 and
full bipolar context at snapshot 1016 differ by more scale than a continuous
1 percent-per-frame camera move can traverse. A declared cut is a normal
cinematic edit and is preferable to either hiding that jump or mislabeling a
lagging path as complete.

Motion derivatives and the path scale limiter reset only at declared cuts. The
diagnostics retain `cut_from_previous` and the one-step
`required_zoom_to_target_percent`, so the magnitude and reason for every edit
remain auditable. All non-cut rows retain the ordinary roll, zoom, and final
target-scale gates.

## Shot schema

The required header is:

```text
# schema=stellar_cinematic_shots_v061
```

Each row has 16 whitespace-separated columns:

```text
name start end mode subject azimuth_deg elevation_deg margin orbit_deg orbit_period_s lobe_sign extent_source screen_half_extent_override_cm transition_kind transition_end easing
```

`extent_source` is `filtered` or `raw`. A positive
`screen_half_extent_override_cm` directly fixes the authored screen half extent
for that shot; `-` uses the selected physical extent and margin. This is a
framing control, not a Cartesian camera coordinate, and is intended for exact
binding to a reviewed anchor composition.

The first shot must use `transition_kind=none` with `- -` for transition end
and easing. Every later shot must declare either:

- `continuous END EASING`, with an incoming window from the shot start through
  the exact end snapshot; or
- `cut - -`, which switches to the new pose at the shot's first snapshot.

Raw framing uses the v060 physical formulas without the filtered tracker's
minimum-extent floor. Filtered framing retains the v054 defaults, preserving
legacy anchor behavior. A scale override takes precedence over either source.

## Review contract

The direction manifest records every override and transition kind, the number
of declared cuts, scale-limited rows, target error, and the existing independent
raw occupancy diagnostics. The SVG preview marks cuts with vertical dashed
lines. Rejected production requests preserve their manifest, diagnostics, and
preview but do not write a camera path.

`ArepoRT` accepts `stellar_cinematic_direction_manifest_v061` and emits
`STELLAR_CAMERA_MANIFEST_V061`; native pose selection continues to emit the
v055 position, look-at, up, and half-extent marker.
