# Stellar GPU physical renderer v053k

`stellar_gpu_renderer_v053k` retains the v053h diagnostic-channel renderer and
adds `hero_adaptive_copper_blue_v081`. It consumes the repaired v080 entry-
relative or unlimited v073 ray payloads; positive legacy absolute `rayMaxT`
payloads remain rejected for the hero program.

The manifest schema is `stellar_gpu_view_manifest_v053k`. Its final six columns
are:

```text
render_program
reference_quantile
response_floor_fraction
material_display_gain
outflow_display_gain
display_contract_sha256
```

`tools/build_stellar_gpu_v053k_projection_manifests.py` writes a no-clobber
display contract with per-view p99 normalization, a two-percent floor, and
dominant-cell projection. The reviewed pose opacity supplies both gains; all
other reviewed camera and style bindings are preserved exactly. No calibration
probe or parameter wave is needed.

Focused build checks:

```bash
c++ -std=c++11 -O2 -Isrc tests/test_stellar_hero_projection_v081.cpp \
  -o build/test_stellar_hero_projection_v081
c++ -std=c++11 -O2 -Isrc tests/test_stellar_gpu_physical_contract_v053k.cpp \
  -o build/test_stellar_gpu_physical_contract_v053k
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -Isrc \
  src/stellar_gpu_renderer_v053k.cu -o build/stellar_gpu_renderer_v053k
```

The first acceptance product is one matched 43-pose still set using the already
repaired v080 scene/ray payloads. No scene export, calibration probe, movie,
channel campaign, or parameter sweep is part of this handoff.
