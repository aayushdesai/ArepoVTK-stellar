# Stellar cinematic director v060

Version v060 separates physical framing from temporal smoothing. It is additive:
v056 and v059 inputs and behavior are unchanged, and accepted v060 paths retain
the `stellar_camera_path_v055` format consumed by `ArepoRT`.

## Shot schema

The required header is:

```text
# schema=stellar_cinematic_shots_v060
```

Each non-comment row has 14 whitespace-separated columns:

```text
name start end mode subject azimuth_deg elevation_deg margin orbit_deg orbit_period_s lobe_sign extent_source transition_end easing
```

`extent_source` is `filtered` or `raw`. Filtered framing uses the temporally
filtered material, disk, and outflow envelopes from v054. Raw framing uses the
current landmark row and these versioned formulas:

```text
material = 1.20 * material_radius
disk     = max(0.80 * material, 1.30 * disk_radius)
outflow  = max(1.15 * disk_radius, 0.58 * polar_extent)
```

The v060 CLI defaults `minimum_half_extent_cm` to `1e8` cm as a numerical floor
when a raw envelope is selected. The raw diagnostic columns retain the physical
formula values before that floor. Older director defaults are unchanged.

## Scale limiter

The authored shot and transition first produce a target screen half extent. The
path then limits the logarithmic scale change from the preceding output row to
`log(1 + max_zoom_percent_per_frame / 100)`. Position is moved along the
authored view direction so its distance remains four screen half extents.

Diagnostics record the target scale, target error, every limited row, filtered
occupancy, and independently computed raw occupancy and clipping. Production is
rejected unless roll and zoom satisfy their budgets and the final scale is
within `max_final_target_scale_error_percent` of the authored target. A smooth
path that never reaches the final outflow framing therefore cannot pass.

Defaults are 0.25 degrees of roll per frame, 1 percent zoom per frame, and 5
percent final target-scale error. Dry runs always preserve diagnostics,
manifest, and SVG review output. A rejected production request preserves those
review products but does not write a camera path.

## Native provenance

The v060 manifest schema is
`stellar_cinematic_direction_manifest_v060`. `ArepoRT` accepts it alongside
v056 and v059 and emits `STELLAR_CAMERA_MANIFEST_V060`. Native pose selection
continues to emit the v055 position, look-at, up, and half-extent audit marker.
