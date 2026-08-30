# Stellar hero projection v080

`hero_projected_copper_blue_v080` replaces the failed v079 hero experiment; it
does not alter or replace the 24 v071/v072 diagnostic channels. The program is
an exact Voronoi **physical-moment projection**, not radiative transfer. The
copper-blue lookup table is diagnostic color and is therefore applied once per
output ray rather than treated as a per-cell emission spectrum.

The renderer accumulates four line-of-sight moments:

```text
Sigma_material = integral rho * max(rotation_in_reviewed_range, hot_support) ds
Sigma_rotation = integral rho * material_support * styled_rotation ds
Sigma_hot      = integral rho * hot_support ds
Phi_outflow    = integral rho * max(v_axial_outward, 0) * coherence^2 ds
```

`hot_support` is a smooth temperature response from log10(T/K)=8.0 to 8.8. It
keeps dense white-dwarf cores visible even when their local velocity
decomposition falls outside the reviewed rotational-fraction range. The
outflow moment is independent of rotational visibility so both coherent lobes
remain available. There are no merger/disk/polar masks, epoch labels, or
time-dependent weights.

After traversal, material and outflow columns are log-compressed against two
frozen references. The density-weighted mean rotational fraction selects the
copper-blue material color; the hot fraction mixes a warm core accent; the
outflow uses the blue stop. A two-percent aggregate-column floor suppresses
the low-density volume that the sparse WebGL renderer does not display.

The four manifest calibration values are:

```text
material_column_reference_g_cm2
material_display_gain             # recommended fixed value 0.22
outflow_column_flux_reference_g_cm_s
outflow_display_gain              # recommended fixed value 0.35
```

Only the two physical references are measured. The display gains are explicit,
fixed source-contract values, not a sweep. Representative v053j probe reports
are frozen with `tools/freeze_stellar_projection_calibration_v080.py`; absent
material or outflow signal at one epoch is recorded as zero and excluded from
that component's reference, with at least two positive epochs required.

## Ray-depth correction

GPU scene export now accepts `stellarGpuRayTraversalLengthCm`. Unlike legacy
`rayMaxT`, this is measured from the simulation-box entry point. The two values
are mutually exclusive. V073 payloads record
`AREPO_STELLAR_ENTRY_RELATIVE_RAY_LIMIT_V080`; v053j rejects a positive legacy
absolute limit for v080 because it can end a distant camera ray before the box.

For reviewed gallery re-export, either use:

```text
rayMaxT = 0
stellarGpuRayTraversalLengthCm = 0
```

for the full box, or set an explicit positive entry-relative traversal length.
The exporter reports no-box, depth-limit, and entry-cell inactive-ray counts.

This correction requires fresh ray payloads for the ten reviewed camera
epochs. The immutable cell/edge data and reviewed camera geometry do not
change.
