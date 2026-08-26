# Stellar cinematic director v062

Version v062 adds hash-bound perceptual framing calibration to the accepted
v061 camera language. Physical landmarks, artistic shot direction, and visual
evidence remain separate inputs. The output is still the native
`stellar_camera_path_v055` table; no Cartesian camera coordinates are authored.

## Why calibration is separate

The v061 pilot proved that geometric extents do not predict visible frame fill
under a transfer function. In the accepted 532-frame audit, the disk shot had
raw occupancy below one while every rendered frame touched the image edge, and
the late bipolar shot retained fixed raw occupancy while its luminous area
contracted to a few percent of the image. Replacing those errors with new
hand-tuned margins would preserve the guesswork.

V062 instead consumes sparse, low-resolution probe measurements. Each row binds
an observed visual fill to the exact source-frame SHA-256 and states the desired
fill. The director derives a half-extent multiplier, interpolates it in log
space within each named shot, blends it across continuous transitions, and
resets normally at declared cuts. The ordinary path zoom limiter and final
target-closure gate remain active after calibration.

## Shot schema

The shot table remains the 16-column v061 language under a new schema:

```text
# schema=stellar_cinematic_shots_v062
name start end mode subject azimuth_deg elevation_deg margin orbit_deg orbit_period_s lobe_sign extent_source screen_half_extent_override_cm transition_kind transition_end easing
```

The first shot uses `none - -`; later shots declare `continuous END EASING` or
`cut - -`. Raw and filtered extent semantics, explicit scale overrides, cut
derivative resets, and all v061 motion diagnostics are unchanged.

## Visual calibration schema

The visual calibration is a separate required file:

```text
# schema=stellar_visual_framing_calibration_v062
snapshot shot_name metric observed_fill target_fill source_frame_sha256
```

Supported metrics are:

- `perceptual_area`
- `largest_component_area`
- `perceptual_bbox`
- `largest_component_bbox`

Area fills use `sqrt(observed_fill / target_fill)` because image area scales as
the inverse square of screen half extent. Bounding-box fills use
`observed_fill / target_fill`. Values must be in `(0, 1]`. SHA-256 values are
lowercase 64-character hexadecimal strings.

Rows are strictly ordered by snapshot. Every named shot requires at least one
row, every row must bind an exact landmark inside that shot, and all rows for a
shot must use one metric. Before the first and after the last probe in a shot,
the nearest correction is held constant. Between probes, the correction is
piecewise linear in log scale using landmark time, not snapshot-number spacing.
This supports a few representative probes rather than a full movie render.

## Command

```text
stellar_camera_director_v062 \
  --landmarks stellar_landmarks.tsv \
  --shots shots_v062.tsv \
  --visual-calibration visual_calibration_v062.tsv \
  --path camera_path.tsv \
  --diagnostics camera_diagnostics.tsv \
  --manifest camera_manifest.txt \
  --preview camera_preview.svg
```

Use `--dry-run` instead of `--path` for no-clobber planning. Dry runs read no
snapshots. The diagnostics record the metric, probe bracket, interpolation
fraction, applied scale factor, target fill, ordinary raw/filtered occupancy,
clipping, roll, zoom, and closure. The manifest records every measurement and
source-frame hash.

## Review contract

Calibration is not self-validating science. Probe images must use the intended
camera, transfer function, palette, and renderer. A new path is accepted only
after its motion gates pass and a small matched contact pilot confirms the
visual-fill targets. Full production remains blocked until that review.

`ArepoRT` accepts `stellar_cinematic_direction_manifest_v062` and emits
`STELLAR_CAMERA_MANIFEST_V062`; native pose selection continues to emit the
v055 position, look-at, up, and half-extent marker.
