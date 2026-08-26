# Stellar portable scene format v052a

## Purpose

The v052a format makes an exported scene sufficient to reproduce the native
stellar material classifier. The previous v041 scene stored position, density,
and temperature only. It could not distinguish rotating disk gas from radial
material or coherent outward polar flow from inflow.

The old header and v051 GPU renderer remain in the repository as compatibility
fixtures. A v041 scene is never interpreted as v052.

## Records

The packed 208-byte header records:

- magic `ARVTKSTARV052A` and format version 5;
- exact record sizes and endian marker;
- image and source dimensions;
- field-presence and unit flags;
- cell, edge, and ray counts;
- box, ray, camera, and snapshot-time metadata;
- explicit position, density, velocity, and temperature unit scales.

Each packed 52-byte cell stores position, `log10(rho)+10`, temperature in K,
the full velocity vector in cm/s, and a 64-bit particle ID. Keeping these values
in one record prevents a later AREPO reorder from separating sidecar fields
from their owning cell.

## Export interface

The native renderer exports one scene with:

```text
AREPORT_STELLAR_EXPORT=/path/to/scene.bin
AREPORT_STELLAR_WIDTH=1920
AREPORT_STELLAR_HEIGHT=1080
```

For multiple camera frames, set `AREPORT_STELLAR_EXPORT_DIR`. The first file
contains the mesh and rays; later files contain compatible ray payloads only.
Existing files are never overwritten.

## GPU view manifest

Each non-comment row contains:

```text
index scene palette output mode
center_x center_y center_z axis_x axis_y axis_z
bulk_vx bulk_vy bulk_vz material_radius disk_radius disk_half_thickness
polar_inner polar_outer polar_cone_ratio
merger_extinction disk_extinction polar_extinction
merger_emissivity disk_emissivity polar_emissivity
exposure black_point saturation
```

The v052 renderer normalizes the axis, rejects malformed physical parameters,
interpolates density, temperature, and velocity with identical weights, and
uses the shared v052 analytic segment integrator.

## Validation

Phase 3 requires binary-layout round trips, field-presence checks, kinematic
disk/outflow fixtures constructed directly from scene cells, native and CUDA
builds, exact legacy images, deterministic rerenders, and a zero diff for the
native Voronoi traversal. Snapshot export and rendered CPU/GPU comparisons are
separate eta pilot products with source and binary hashes.
