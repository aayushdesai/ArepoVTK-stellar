# Stellar GPU physical renderer v053j

`stellar_gpu_renderer_v053j` retains v053h for the 24 diagnostic channels and
adds one opt-in program: `hero_projected_copper_blue_v080`. Historical v079 is
preserved in v053i and is intentionally not accepted by v053j.

The manifest schema is `stellar_gpu_view_manifest_v053j`. Its 58-column order
matches v053i, but the final six columns are defined as:

```text
render_program
material_column_reference_g_cm2
material_display_gain
outflow_column_flux_reference_g_cm_s
outflow_display_gain
projection_calibration_sha256
```

The program requires rotational_fraction, linear scale, a reviewed range
within `[0,1]`, composite transfer, `copper_blue_accent_v058`,
`stellar_structures_v065`, `legacy_v072` optical profile, and exact Voronoi
reconstruction. The optical profile is retained only to keep the shared
manifest surface stable; v080 does not call the optical integrator.

Use `tools/build_stellar_gpu_v053j_projection_manifests.py` to preserve the
exact 43 camera alternatives and load the latest reviewed style binding for
each pose. It supports a three-pose calibration package and a final 43-pose
package, refuses clobber, and records all hashes. The first acceptance run must
use repaired ray payloads and only the 43 matched stills. No movie or parameter
wave is part of this source handoff.

Focused build checks:

```bash
c++ -std=c++11 -O2 -Isrc tests/test_stellar_hero_projection_v080.cpp \
  -o build/test_stellar_hero_projection_v080
c++ -std=c++11 -O2 -Isrc tests/test_stellar_gpu_physical_contract_v053j.cpp \
  -o build/test_stellar_gpu_physical_contract_v053j
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -Isrc \
  src/stellar_gpu_renderer_v053j.cu -o build/stellar_gpu_renderer_v053j
```

The A100 run is render-only and consumes checksum-verified v073 scenes and
repaired ray payloads. Scene export, calibration interpretation, and visual
audit remain eta work.
