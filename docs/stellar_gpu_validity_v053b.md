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

The consolidated Phase 20 validator retains these unit gates, exact legacy
images, deterministic products, unchanged Voronoi traversal, and CUDA build
coverage. It does not execute a GPU. V053b remains parity evidence; v053c adds
the profile-bound preview interface. Promotion still requires native rendering
and an independent no-clobber GPU runtime parity audit.
