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

The v053 candidate exposes `sph`, `idw`, and piecewise `voronoi` at runtime.
All modes use valid first-ring Voronoi neighbors, aligned density/temperature/
velocity weights, exact-center handling, and deterministic parent-cell
fallback. The CUDA reader uses the same topology and mode semantics.

## Phase 5: camera direction

Represent tracked center, orbital plane, angular-momentum axis, disk radius,
and polar extent as smoothed physical landmarks. Add shot constraints, scale
hysteresis, bounded acceleration, and deterministic camera paths.

The v054 candidate keeps material, disk, and polar extents independent; makes
the angular-momentum sign continuous; parallel-transports the camera roll
reference; and bounds center acceleration, axis rate, and logarithmic zoom
rate. It emits no-clobber camera tables for disk, outflow, lobe-following, and
orbit shots without reading snapshots in the planner.

## Phase 6: campaign and release

Benchmark complete campaign makespan, freeze availability-first scheduling,
document the production API, publish fixtures, and require coverage, decoding,
and checksum audits for release candidates.

The v055 candidate loads strict snapshot-indexed v054 camera tables directly
in the native renderer. It rejects malformed geometry, missing snapshots, and
legacy-keyframe conflicts; updates position, look-at, up, and orthographic
scale before camera creation; and preserves the legacy path when disabled. The
production contract freezes scientific and cinematic inputs separately and
defines the evidence required to promote a movie campaign.
