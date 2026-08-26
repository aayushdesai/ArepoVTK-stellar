# Stellar reconstruction v053

## Scope

The v053 reconstruction layer changes how density, temperature, and velocity
are evaluated inside a traversed AREPO cell. It does not replace ray entry,
Voronoi face intersection, or cell-to-cell traversal. The upstream
`voronoi_3db.cpp` and `voronoi_3db.h` implementation remains the common
reference for every mode.

## Runtime modes

Native configurations select one mode with `stellarReconstruction`:

- `sph`: the historical ArepoVTK cubic-spline weighting over valid first-ring
  Voronoi neighbors. `stellarSphSupportFactor` defaults to `1.2`, preserving
  the historical `HSML_FAC` behavior.
- `idw`: inverse-distance weighting over the same first-ring neighbors.
  `stellarIdwPower` defaults to `2.0`. Exact cell-center samples bypass the
  singular weight and return that cell's fields.
- `voronoi`: piecewise-constant native-cell values from the containing cell.
  This is the least smoothed representation of AREPO's finite-volume state.

All reconstructed fields use one normalized weight set. Temperature and the
full velocity vector therefore remain aligned with density and stable cell
identity. Empty or non-finite supports fall back deterministically to the
containing cell.

The v053 CUDA reader consumes the Phase 3 v052 scene format and selects the
same three modes per manifest row. Its neighbor cache is first-ring only so
CPU/GPU comparisons use the same support topology.

## Validation order

Reconstruction is accepted in this order:

1. Analytic weight parity with the historical SPH kernel.
2. Exact-sample, constant-field, normalized-vector, symmetry, and finite
   fallback fixtures.
3. Native and CUDA builds with all Phase 0-3 gates retained.
4. Snapshot field-integral and physical-landmark comparison on eta.
5. Matched-view contact sheets and short movies for `sph`, `idw`, and
   `voronoi` using identical transfer and camera parameters.

Image preference cannot override a failed field-integral or landmark gate.
The purpose of the comparison is to quantify whether smoothing creates the
diffuse fluff seen in production while retaining the disk and polar outflow.
