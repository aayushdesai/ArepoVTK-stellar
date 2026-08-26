# Stellar render model v052a

## Scope

Phase 2 separates local emissivity from extinction while retaining the native
AREPO Voronoi traversal as the geometry reference. The v051 model used one
layer coefficient both to make gas visible and to attenuate material behind
it. That coupling makes diffuse outflow disappear when opacity is lowered and
makes a disk become a smooth opaque surface when visibility is increased.

The v052 optical sample contains:

- RGB emissivity per unit path length;
- scalar extinction per unit path length.

Each merger, disk, and outflow layer has independent emissivity and extinction
coefficients. Existing opacity keys remain valid. New emission keys default to
their corresponding opacity values, preserving the familiar source-function
behavior for single-layer shots while allowing optically thin emission to be
tuned independently.

## Segment equation

For constant emissivity `j`, extinction `alpha`, and segment length `ds`, the
integrator evaluates the analytic solution

`dI/ds = j - alpha I`.

The segment transmittance is `exp(-alpha ds)`. Its emitted radiance is
`j * (1 - exp(-alpha ds)) / alpha`, with the continuous optically thin limit
`j * ds` when `alpha` is zero. This avoids step-size-dependent source terms and
supports emitting material with zero extinction.

## Configuration

The native renderer accepts these additional optional keys:

- `stellarMergerEmission`
- `stellarDiskEmission`
- `stellarPolarEmission`

All three must be positive. The existing `stellarMergerOpacity`,
`stellarDiskOpacity`, and `stellarPolarOpacity` values may be zero but not
negative.

## Validation gate

`slurm/phase2_validate.sbatch` must prove:

- unchanged deterministic upstream reference images;
- passing v051 classification/color tests;
- passing v052 emissivity/extinction and analytic segment tests;
- successful native and CUDA regression builds;
- unchanged `src/voronoi_3db.cpp` and `src/voronoi_3db.h` relative to upstream.

GPU behavior remains at the v051 scene contract in this phase. Velocity-aware
CPU/GPU parity belongs to Phase 3 and must version the scene format rather than
silently reinterpret existing scene files.
