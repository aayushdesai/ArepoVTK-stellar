# Stellar cinematic director v063

Version v063 makes the v062 perceptual framing loop iterative and
source-camera aware. Physical landmarks, artistic shot direction, and visual
evidence remain separate inputs. The output is still the native
`stellar_camera_path_v055` table; no Cartesian camera coordinates are authored.

## Why calibration is separate

The v061 pilot proved that geometric extents do not predict visible frame fill
under a transfer function. In the accepted 532-frame audit, the disk shot had
raw occupancy below one while every rendered frame touched the image edge, and
the late bipolar shot retained fixed raw occupancy while its luminous area
contracted to a few percent of the image. Replacing those errors with new
hand-tuned margins would preserve the guesswork.

V062 exposed one remaining provenance gap: it bound the source image but not
the camera scale that produced it. A correction from a second contact pilot
would therefore be applied relative to the original physical path instead of
compounding from the rendered source path.

V063 binds each sparse observation to its exact source-frame SHA-256, source
camera-path SHA-256, and source screen half extent. It first derives an
absolute desired half extent, interpolates that absolute target in log space,
and only then rebases it onto the current physical camera pose. This preserves
the meaning of repeated calibration passes without manually multiplying zoom
factors. Continuous transitions blend absolute targets; declared cuts reset
normally. The ordinary path zoom limiter and final target-closure gate remain
active after calibration.

## Shot schema

The shot table remains the 16-column v061 language under a new schema:

```text
# schema=stellar_cinematic_shots_v063
name start end mode subject azimuth_deg elevation_deg margin orbit_deg orbit_period_s lobe_sign extent_source screen_half_extent_override_cm transition_kind transition_end easing
```

The first shot uses `none - -`; later shots declare `continuous END EASING` or
`cut - -`. Raw and filtered extent semantics, explicit scale overrides, cut
derivative resets, and all v061 motion diagnostics are unchanged.

## Visual calibration schema

The visual calibration is a separate required file:

```text
# schema=stellar_visual_framing_calibration_v063
snapshot shot_name metric observed_fill target_fill source_screen_half_extent_cm source_frame_sha256 source_camera_path_sha256
```

Supported metrics are:

- `perceptual_area`
- `largest_component_area`
- `perceptual_bbox`
- `largest_component_bbox`

Area observations produce
`source_screen_half_extent_cm * sqrt(observed_fill / target_fill)` because
image area scales as the inverse square of screen half extent. Bounding-box
observations produce
`source_screen_half_extent_cm * observed_fill / target_fill`. Fill values must
be in `(0, 1]`; source half extents must be finite and positive. Both SHA-256
values are lowercase 64-character hexadecimal strings.

Rows are strictly ordered by snapshot. Every named shot requires at least one
row, every row must bind an exact landmark inside that shot, and all rows for a
shot must use one metric. Before the first and after the last probe in a shot,
the nearest absolute target is held constant. Between probes, source half
extent and absolute target half extent are piecewise linear in log scale using
landmark time, not snapshot-number spacing. This supports a few representative
probes rather than a full movie render.

## Command

```text
stellar_camera_director_v063 \
  --landmarks stellar_landmarks.tsv \
  --shots shots_v063.tsv \
  --visual-calibration visual_calibration_v063.tsv \
  --path camera_path.tsv \
  --diagnostics camera_diagnostics.tsv \
  --manifest camera_manifest.txt \
  --preview camera_preview.svg
```

Use `--dry-run` instead of `--path` for no-clobber planning. Dry runs read no
snapshots. The diagnostics record the metric, probe bracket, interpolation
fraction, effective rebase factor, target fill, source half extent, absolute
target half extent, ordinary raw/filtered occupancy, clipping, roll, zoom, and
closure. The manifest records every measurement, source-frame hash, and source
camera-path hash.

## Review contract

Calibration is not self-validating science. The sidecar source half extent must
match the row for the declared snapshot in the exact source camera path whose
SHA-256 is recorded. Operations must audit that binding alongside the frame,
transfer function, palette, and renderer. A new path is accepted only after its
motion gates pass and a small matched contact pilot confirms the visual-fill
targets. Full production remains blocked until that review.

`ArepoRT` accepts `stellar_cinematic_direction_manifest_v063` and emits
`STELLAR_CAMERA_MANIFEST_V063`; native pose selection continues to emit the
v055 position, look-at, up, and half-extent marker.
