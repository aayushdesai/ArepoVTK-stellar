# Stellar GPU supported physical renderer v053e

`stellar_gpu_renderer_v053e` retains the v053d v073 scene, all 24 physical
channels, native Voronoi traversal, and copper-blue color contract. Its manifest
adds the opt-in v074 optical profile and normalization parameters.

```text
# schema=stellar_gpu_view_manifest_v053e
```

After the seven v053d physical columns, append:

```text
physical_optical_profile target_optical_depth target_emission reference_path_cm
```

Use `legacy_v072` to reproduce the old transfer. Use
`material_support_v074 1 1 0` for feature-weighted support and an automatically
resolved per-channel reference path. Every benchmark records the requested
profile, dimensionless targets, resolved reference path, and whether zero scalar
signal is transparent.

Build-only handoff:

```bash
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -Isrc \
  src/stellar_gpu_renderer_v053e.cu -o build/stellar_gpu_renderer_v053e
c++ -std=c++11 -O2 -Isrc tests/test_stellar_gpu_physical_contract_v053e.cpp \
  -o build/test_stellar_gpu_physical_contract_v053e
```

GPU execution is not a source-level acceptance gate. Operations must first run
the focused contract and determinism checks, then render the exact 43-camera
rotational-fraction set and pass the v074 visual audit before any movie or
profile promotion.
