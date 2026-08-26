# Stellar renderer roadmap

## Phase 0: reproducible baseline

Freeze upstream commit, dependency versions, deterministic reference images,
field-registry fixtures, source hashes, and eta-only validation provenance.

## Phase 1: stellar semantics

Add stellar field registration, protected temperature and velocity alignment,
kinematic merger/disk/outflow classification, fixed exposure, and deterministic
tone mapping. Keep native Voronoi traversal unchanged.

## Phase 2: radiative transfer

Separate emissivity from extinction and validate the analytic constant-segment
solution. Preserve the v051 model and tests as a comparison fixture.

## Phase 3: portable physical scenes

Version the scene format to carry vector velocity, stable cell identity, units,
and field-presence flags. Require CPU/GPU classification parity on analytic and
three-snapshot fixtures before GPU production.

The v052a implementation keeps v041 readers intact, adds a separate packed
version-5 contract, exports aligned velocity and temperature from the native
mesh, and shares the v052 transfer and segment functions with its CUDA reader.
Binary layout and scene-cell kinematic fixtures are mandatory before any
snapshot pilot is accepted.

## Phase 4: reconstruction

Define a runtime reconstruction interface around the unchanged Voronoi
traversal. Keep the SPH-kernel path as the current reference, repair and test
IDW, and add a Voronoi-native piecewise or gradient reconstruction. Compare
field integrals and landmarks before image metrics.

## Phase 5: camera direction

Represent tracked center, orbital plane, angular-momentum axis, disk radius,
and polar extent as smoothed physical landmarks. Add shot constraints, scale
hysteresis, bounded acceleration, and deterministic camera paths.

## Phase 6: campaign and release

Benchmark complete campaign makespan, freeze availability-first scheduling,
document the production API, publish fixtures, and require coverage, decoding,
and checksum audits for release candidates.
