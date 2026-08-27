# Stellar feature diagnostics v064

## Purpose

`stellar_feature_probe_v064` measures the native stellar transfer selectors on
an existing packed v052 scene. It is an instrumentation tool, not a new render
mode. The merger, disk, and polar equations remain in
`stellar_render_model_v052a.h`; the renderer calls the same extracted feature
function used by the probe.

The first use is to distinguish two failure classes found by the Phase 14
native-layer pilot:

- disk-selected cells may overlap the high-density merger/core envelope;
- polar-selected cells may extend beyond the camera even when the landmark
  radius predicts acceptable framing.

No selector coefficient is changed in v064.

## Inputs and contract

The probe accepts a packed `ARVTKSTARV052A` scene and the exact physical
tracking values used to export/render that scene. Required named options are:

```text
--scene PATH --output PATH
--center X Y Z --axis X Y Z --bulk VX VY VZ
--box-size CM --material-radius CM
--disk-radius CM --disk-half-thickness CM
--polar-inner CM --polar-outer CM --polar-cone-ratio VALUE
```

`--minimum-weights` optionally supplies a comma-separated threshold list. The
default is `0.01,0.05,0.10,0.25,0.50`.

The implementation rejects malformed geometry, non-finite cell fields,
non-orthographic ray grids, unsupported units, truncated scenes, and an
existing output path. It does not read an AREPO snapshot and does not traverse
the mesh.

## Output

The no-clobber TSV contains one row per feature and threshold for:

- selected cell count and total selector weight;
- weighted merger overlap, high-density fraction, and inner-radius fraction;
- weighted median density, normalized radius/height, and rotational fraction;
- projected cell-center width and height relative to the exported camera grid;
- edge-contact and right-censoring flags.

`polar_positive` and `polar_negative` are reported separately. Projected
support is a cell-center lower bound, not a rendered-pixel or Voronoi-cell
footprint. A right-censored row is evidence that the selected support reaches
the exported field boundary; its reported extent must not be used as a finite
camera correction.

## Validation and use

Phase 15 validation requires:

- exact retained beauty-image hashes and an empty Voronoi reference diff;
- deterministic repeated probe fixtures;
- explicit evidence that the legacy disk selector can overlap merger/core
  material;
- separate positive and negative polar support;
- strict malformed-input and no-clobber failures;
- native and CUDA renderer builds, with no GPU execution.

Scientific use must run on eta from an immutable scene export with the scene,
binary, camera, tracking, transfer, and report hashes preserved. Selector or
camera changes require a later version and a separate visual pilot.
