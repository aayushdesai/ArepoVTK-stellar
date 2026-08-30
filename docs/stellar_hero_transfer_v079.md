# Stellar hero transfer v079

`hero_material_outflow_v079` is an opt-in render program beside the retained
v078 physical-channel renderer. It does not add a twenty-fifth physical
channel and does not change the existing 24 channel formulas, scene fields, or
v053h renderer.

The hero program uses two per-cell physical terms and one Beer-Lambert ray
integrator:

```text
alpha_material = tau_ref * rho / Sigma_ref
flux_outward = rho * max(v_axial_outward, 0) * coherence^2
j = alpha_material * thermal_source * copper_blue(rotation, temperature)
  + outflow_gain * flux_outward / Phi_ref * cobalt
```

`Sigma_ref` has units `g cm^-2`; `Phi_ref` has units `g cm^-1 s^-1`.
Both are frozen from representative ray columns. The material term carries
white dwarfs, remnant, disk, and tails through actual mass column. The outflow
term carries both signed lobes through coherent outward mass flux. There are no
epoch labels and no merger/disk/polar support masks.

The four explicit hero inputs are:

```text
stellarHeroMaterialColumnReference
stellarHeroMaterialOpticalDepth
stellarHeroOutflowColumnFluxReference
stellarHeroOutflowEmission
```

`stellarExposure` remains the final display exposure. Hero configs also require
`stellarHeroCalibrationSHA256`, `stellarPhysicalChannel=rotational_fraction`,
`stellarPhysicalScale=linear`, a reviewed subrange within `[0,1]`,
`stellarPaletteProfile=copper_blue_accent_v058`, composite transfer, and native
Voronoi reconstruction.

## GPU contract

`stellar_gpu_renderer_v053i` consumes the same immutable v073 scenes. Its
58-column manifest keeps the first 52 v053h columns unchanged and appends:

```text
render_program
material_column_reference_g_cm2
material_optical_depth_at_reference
outflow_column_flux_reference_g_cm_s
outflow_emission_at_reference
hero_calibration_sha256
```

The renderer records positive-ray material-column p50/p90/p95 and
outflow-column-flux p50/p90/p95 in each benchmark. The intended bounded
workflow is:

1. Build a three-pose `--calibration-probe` package with
   `tools/build_stellar_gpu_v053i_hero_manifests.py`.
2. Freeze one epoch-balanced calibration JSON with
   `tools/freeze_stellar_hero_calibration_v079.py`.
3. Build one exact 43-pose hero package and compare it to the reviewed WebGL
   controls. Do not render a movie before those stills pass.

Build the focused sources with:

```bash
c++ -std=c++11 -O2 -Isrc tests/test_stellar_hero_transfer_v079.cpp \
  -o build/test_stellar_hero_transfer_v079
c++ -std=c++11 -O2 -Isrc tests/test_stellar_gpu_physical_contract_v053i.cpp \
  -o build/test_stellar_gpu_physical_contract_v053i
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -Isrc \
  src/stellar_gpu_renderer_v053i.cu -o build/stellar_gpu_renderer_v053i
```

Magnetic field, magnetic pressure, gas pressure, plasma beta, Alfven speed,
sound speed, Mach number, and entropy proxy remain available through the
unchanged v071/v072 diagnostic-channel path. They are intentionally not folded
into the initial hero emissivity because doing so would imply an unvalidated
magnetic or thermodynamic radiation mechanism.
