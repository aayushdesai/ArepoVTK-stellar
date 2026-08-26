# Stellar cinematic director v056

## Purpose

The v056 director replaces camera-coordinate guesswork with two explicit,
versioned inputs:

- a physical landmark table produced from snapshots; and
- an artistic shot table written in scientific and cinematographic terms.

The director converts those inputs into a strict v055 native camera path. It
also emits diagnostics, a manifest, and an SVG dry-run preview. Planning reads
only compact landmark tables. It does not read snapshots or render images.

## Shot specification

The shot table starts with:

```text
# schema=stellar_cinematic_shots_v056
```

Each non-comment row has 13 whitespace-separated columns:

```text
name start_snapshot end_snapshot mode subject azimuth_deg elevation_deg framing_margin orbit_degrees orbit_period_seconds lobe_sign transition_seconds easing
```

`name` and `subject` are stable provenance labels. Snapshot ranges must be
ordered, contiguous, unique by name, and cover the landmark table exactly.

`mode` is one of `disk_edge`, `disk_oblique`, `outflow_side`,
`outflow_axis`, `outflow_follow`, or `orbit`. Azimuth and elevation are explicit
degrees in the tracked physical basis. `framing_margin` multiplies the
mode-specific tracked extent.

Specify either a total `orbit_degrees` over the shot or an
`orbit_period_seconds`; use `-` for the unused control. They are independent of
`lobe_sign`, so selecting the positive or negative polar lobe never requires a
dummy orbit period. `lobe_sign` is `1` or `-1`.

`transition_seconds` blends the end of the current shot into the next shot.
The final shot must use zero. `easing` is `linear`, `smoothstep`, or
`smootherstep`. A zero transition is an intentional hard cut.

Example:

```text
# schema=stellar_cinematic_shots_v056
approach 0 320 orbit binary 28 22 1.08 35 - 1 80 smootherstep
disk_reveal 321 700 disk_oblique remnant_disk 42 35 1.05 - - 1 100 smootherstep
polar_follow 701 985 outflow_follow positive_lobe 20 8 1.10 - - 1 0 smoothstep
```

## Named command interface

Create a native path and all planning evidence:

```bash
stellar_camera_director_v056 \
  --landmarks landmarks.tsv \
  --shots shots.tsv \
  --path camera_path.tsv \
  --diagnostics camera_diagnostics.tsv \
  --manifest camera_direction.manifest \
  --preview camera_preview.svg
```

Inspect a proposed direction without creating a camera path:

```bash
stellar_camera_director_v056 \
  --landmarks landmarks.tsv \
  --shots shots.tsv \
  --dry-run \
  --diagnostics camera_diagnostics.tsv \
  --manifest camera_direction.manifest \
  --preview camera_preview.svg
```

Every output is no-clobber. Output paths must be distinct. Unknown, positional,
duplicate, non-finite, and invalid controls fail before output creation.

All v054 physical-filter controls are exposed as named options:

- `--center-response-seconds`
- `--center-max-speed-cm-per-s`
- `--center-max-acceleration-cm-per-s2`
- `--axis-response-seconds`
- `--axis-max-rate-deg-per-s`
- `--scale-response-seconds`
- `--scale-max-log-rate-per-s`
- `--expand-hysteresis-fraction`
- `--contract-hysteresis-fraction`
- `--minimum-half-extent-cm`

Omitted controls use the tested v054 defaults, which are recorded in the
manifest. This keeps a concise first edit while making every smoothing and
motion assumption inspectable and reproducible.

## Diagnostics and revision loop

The diagnostics table reports per-frame center and camera speed,
acceleration, jerk, axis and roll rates, logarithmic zoom rate and
acceleration, material/disk/outflow occupancy, and clipping flags. The SVG
preview shows the shot timeline, occupancy, and normalized motion rates.

The intended direction loop is:

1. Edit named shots rather than Cartesian coordinates.
2. Run `--dry-run` on the compact landmark table.
3. Inspect transition windows, clipping flags, occupancy, and motion spikes.
4. Revise azimuth, elevation, framing, movement, or easing.
5. Generate the native path only after the plan is acceptable.
6. Render a representative contact sheet before full production.

The SVG is a geometric planning diagnostic, not an image-quality substitute.
Contact sheets remain the acceptance gate for whether a disk or outflow is
actually visible through the chosen transfer function.

## Production provenance

The path output retains the v055 schema and records its landmark and shot
inputs in comments. The sidecar manifest records all shot and filter controls.
Set both in a native renderer config:

```text
stellarCameraPath = /absolute/path/camera_path.tsv
stellarCameraDirectionManifest = /absolute/path/camera_direction.manifest
```

The native renderer validates the manifest schema at startup and logs the
manifest path. Each selected frame logs camera position, look-at, up, and
orthographic scale. Existing v055 paths remain valid without a manifest for
backward compatibility, but a production v056 campaign should always bind both
files and retain their checksums.

## Boundaries

The physical landmark extractor and the artistic director are intentionally
separate. A change in scientific tracking can be evaluated without silently
changing direction, and a new edit can reuse the same physical evidence. The
current C++ Voronoi traversal remains the unchanged reference implementation.
