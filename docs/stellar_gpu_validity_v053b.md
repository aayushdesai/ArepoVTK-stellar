# Stellar GPU validity and display parity v053b

## Scope

`stellar_gpu_renderer_v053b` corrects two promotion-blocking defects found by
the snapshot-1016 CPU/GPU parity pilot:

- Rays that do not intersect the mesh retain `kRayInactive` accounting but do
  not invalidate an otherwise clean view.
- GPU TGA output uses the same `pow(value, 1/2.3)` byte encoding as native
  ArepoVTK PNG/TGA output.
- Voronoi face intersections use the scene's double cell positions, matching
  native traversal instead of reconstructing faces from float-packed deltas.
- SPH reconstruction preserves native connection multiplicity and uses ghost
  connections for support-radius selection without adding ghost field values.

All traversal failures remain fatal: invalid cell, invalid edge, neighbor
overflow, missing exit face, cell limit, non-finite output, and unknown future
status bits. The benchmark report records all seven known counters separately.

## Validation

`tests/test_stellar_gpu_ray_status_v053b.cpp` reproduces the observed 116,082
inactive background rays and verifies that each real error flag still fails.
`tests/test_stellar_gpu_output_parity_v053b.cpp` compares representative
low-surface-brightness pixels against the historical native byte equation and
rejects the obsolete linear GPU quantization.
`tests/test_stellar_gpu_geometry_v053b.cpp` preserves double precision for
large periodic-box coordinates and demonstrates the avoided packed-delta loss.
`tests/test_stellar_gpu_neighbor_reference_v053b.cpp` verifies duplicate local
contributors and support-only ghost references independently.

`slurm/gpu_validity_validate.sbatch` retains every Phase 0-7 gate, exact legacy
images, deterministic products, unchanged Voronoi traversal, and CUDA build
coverage. It does not execute a GPU. Promotion still requires a no-clobber GPU
runtime parity rerun against the preserved native snapshot fixture.
