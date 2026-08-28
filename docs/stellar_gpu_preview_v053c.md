# Stellar GPU preview renderer v053c

`stellar_gpu_renderer_v053c` is a preview-only extension of the validated
v053b inactive-ray, display-encoding, and double-precision face-geometry fixes.
It intentionally excludes the later SPH neighbor-semantics experiment.

The v053c view manifest adds explicit palette and feature-profile names after
the transfer-mode column. This closes the v053b provenance gap that otherwise
left both profile IDs at their zero-valued defaults. Accepted profile names are
the native names through v068 plus the opt-in
`structure_flux_layered_v070`. The structure-flux profiles are rejected unless
the row also selects `composite` transfer and `stellar_structures_v065`.

The manifest begins with exactly this version line:

```text
# schema=stellar_gpu_view_manifest_v053c
```

Each subsequent data row has the following whitespace-delimited columns:

```text
index scene palette output_prefix transfer palette_profile feature_profile reconstruction idw_power sph_support center_x center_y center_z axis_x axis_y axis_z bulk_vx bulk_vy bulk_vz material_radius disk_radius disk_half_thickness polar_inner polar_outer polar_cone_ratio merger_extinction disk_extinction polar_extinction merger_emissivity disk_emissivity polar_emissivity exposure black_point saturation
```

Indices are contiguous from zero. Extra columns, missing or late schema
declarations, unknown names, nonphysical numeric values, and incompatible
profile combinations are rejected before allocating or rendering a view.

The renderer writes the exact palette and feature-profile names and IDs into
each benchmark report and includes them in `GPU_VIEW_OK`. Packed scene format
v052 and `StellarTransferParameters` remain unchanged; profiles are bound by
the separately hashed view manifest.

GPU products are for rapid visual ranking only. A successful GPU status audit
does not promote a transfer profile or camera. Native C++ Voronoi contacts on
the same immutable snapshots remain the scientific acceptance gate until
strict CPU/GPU parity is independently requalified.
