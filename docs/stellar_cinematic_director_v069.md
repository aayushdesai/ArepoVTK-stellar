# Stellar cinematic director v069

V069 applies the signed, robust v067 framing recommendations to a continuous
camera path. It keeps physical landmarks, visual calibration, feature-framing
evidence, and artistic shot direction as separate versioned inputs. It writes
the native `stellar_camera_path_v055` table and never accepts Cartesian camera
coordinates in an authoring file.

## Continuous direction

The shot table extends the retained language with `roll_deg` after
`elevation_deg` under `stellar_cinematic_shots_v069`. Roll is the total
screen-plane rotation accumulated across the shot, so a single no-cut shot can
preserve the reviewed opening and gradually use the 16:9 width for the late
bipolar composition. V069 accepts `none` for the first shot and
`continuous` for later shots. It rejects every declared `cut`; hard cuts and
cross-dissolves are outside this director's release contract. Orbit degrees,
azimuth, elevation, physical subject, extent source, and visual calibration
continue to behave as in v063. The manifest records both the authored
`roll_deg` value and its radians conversion.

```text
# schema=stellar_cinematic_shots_v069
name start end mode subject azimuth_deg elevation_deg roll_deg margin orbit_deg orbit_period_s lobe_sign extent_source screen_half_extent_override_cm transition_kind transition_end easing
```

## Signed framing sidecar

The required sidecar is whitespace-delimited:

```text
# schema=stellar_signed_framing_application_v069
name shot_name source_snapshot transition_start transition_end easing feature_mode features minimum_weight coverage target_half_width_fraction target_half_height_fraction look_at_shift_screen_x_fraction look_at_shift_screen_y_fraction half_extent_scale_factor source_screen_half_extent_cm framing_plan_sha256 source_frame_sha256 source_camera_path_sha256 source_camera_row_sha256
```

`feature_mode` is `single` or `bipolar_union`. Bipolar rows must retain both
canonical v067 members as `polar_positive,polar_negative`; alternate spellings
are rejected. Coverage is the v067 `0.90` or `0.98`
central interval. Every SHA-256 value is required and is copied into the native
direction manifest for operations to verify against the exact v067 plan,
source image, source v055 path, and source path row.

Rows are ordered and their transition windows may not overlap. Before the
first window the signed correction is the identity. Each window interpolates
from the prior held recommendation to the new v067 recommendation with the
declared easing. The physical screen-plane shift is

```text
source_screen_half_extent_cm * look_at_shift_screen_fraction
```

in the current camera right/up basis. Position and look-at move together, so
the view direction is not changed by recentering. The absolute scale target is

```text
source_screen_half_extent_cm * half_extent_scale_factor
```

and is interpolated in log space. The ordinary path-level zoom limiter remains
authoritative after the signed target is applied.

## Motion contract

The release defaults are 0.25 degree roll per frame, 1 percent zoom per frame,
and 0.01 artistic screen-half-extent of pan per frame. Diagnostics distinguish
the authored signed pan from physical landmark tracking and report its step and
acceleration. Production path output is rejected when roll, zoom, pan, or final
scale closure exceeds the named budget.

## Command

```text
stellar_camera_director_v069 \
  --landmarks stellar_landmarks.tsv \
  --shots shots_v069.tsv \
  --visual-calibration visual_calibration_v069.tsv \
  --signed-framing signed_framing_v069.tsv \
  --path camera_path.tsv \
  --diagnostics camera_diagnostics.tsv \
  --manifest camera_manifest.txt \
  --preview camera_preview.svg
```

Use `--dry-run` instead of `--path` for a zero-snapshot feasibility review.
All outputs are no-clobber. `ArepoRT` accepts
`stellar_cinematic_direction_manifest_v069` and emits
`STELLAR_CAMERA_MANIFEST_V069`; native pose selection remains the v055
position, look-at, up, and half-extent marker.

## Acceptance boundary

V069 makes the v067 recommendation reproducible; it does not make a diagnostic
recommendation scientifically or artistically correct. Operations must verify
all bound hashes, then compare two sparse no-cut camera candidates with the
accepted optical profile. Smoothness requires a contiguous dense motion clip,
not a contact sheet. Full 986-frame production remains blocked until those
reviews pass.
