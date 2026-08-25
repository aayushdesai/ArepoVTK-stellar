# Stellar render model v051a

## Purpose

The v051a renderer separates merger material, the rotationally supported disk,
and polar material before compositing. It replaces per-frame minimum/maximum
normalization with fixed exposure, a fixed black point, filmic tone mapping,
and restrained saturation. The native C++ Voronoi traversal remains the
reference geometry path.

## Calibration

The initial thresholds were measured on snapshots 100, 145, 210, 275, 330,
450, 600, 760, 900, and 1016 in eta job 63693512. The compact products are in:

`/nfs/scistore25/caiazgrp/adesai/Arepo/outreach_volume_movie/v031_rework/stellar_v051/diagnostics/phase_calibration_20260825T205831Z`

The post-merger outer disk is typically dense (`log10 rho` about 0.6 to 1.9),
hot (`log10 T/K` about 7.3 to 7.6), and rotation dominated. Mature polar
material is diffuse (`log10 rho` about -3 to -1), cooler (`log10 T/K` about
6.4 to 7.1), and has coherent outward speeds of order 1.5e8 to 6e8 cm/s.

The native path preserves the full snapshot velocity vector alongside the
legacy scalar velocity magnitude and interpolates it with the same Voronoi
weights as density and temperature. After subtracting the tracked bulk
velocity, disk material is selected by rotational support and polar material by
coherent outward axial motion. Geometry supplies only broad, smooth tapers; it
does not define a visible cone or slab boundary. The current GPU scene stores
only position, density, and temperature, so this velocity-aware recipe is native
path only until the scene format gains vector velocity.

## Shot classes

- `merger`: compact thermal material for the binary and coalescence.
- `disk`: rotation-dominated gas around the tracked angular-momentum axis.
- `outflow`: coherent outward axial gas with an independent large spatial scale.
- `composite`: restrained merger, disk, and outflow layers in one context shot.

Disk and outflow shots must not share one camera scale. Disk pilots use fields
of view near 3.5e10 cm. Outflow pilots scale from 5e10 cm to about 5e11 cm using
the tracked polar extent. Camera orientations are held deliberately rather than
orbiting continuously, so changes in morphology remain readable.

## Color and tone

The palette is a restrained physical-temperature sequence from deep red through
gold and warm white to pale blue. Disk and outflow colors are modest warm-gold
and cobalt biases applied to that shared thermal sequence, rather than flat
categorical colors. Teal and pink are excluded. Front-to-back opacity prevents
unrelated line-of-sight material from adding without limit.

## Validation

Velocity-aware validation job 63694350 ran on eta298 with `Arepo_Env`. It proved:

- exact legacy `frame2.png` output;
- deterministic repeated reference renders;
- passing stellar render-model and field-registry unit tests;
- focused coverage for rotating versus radial disk gas, outward versus inward
  polar gas, equatorial tails, and tracked bulk-velocity subtraction;
- successful native and CUDA renderer builds;
- no changes to `src/voronoi_3db.cpp` or `src/voronoi_3db.h`.

The CUDA binary is build-validated only. AREPO rendering remains eta-only under
the workspace execution policy.
