# Supported physical-channel optics v074

`material_support_v074` fixes the white-sheet failure of the v071/v072 scalar
transfer without changing scalar color. The existing `legacy_v072` profile is
the default and retains the exact `0.08 + 0.92 * fraction` amplitude and
per-centimeter coefficients.

The supported profile separates three concepts:

1. the normalized scalar fraction selects the copper-blue color;
2. scalar signal times a v065 material, disk, or polar feature weight controls
   whether the cell contributes optically; and
3. extinction and emission are normalized by a physical reference path for
   that channel class.

A zero rotational fraction therefore has exactly zero extinction and emission.
A nonzero disk-class scalar outside the v065 annular disk support is also
transparent; merger support is not allowed to leak the compact core back into
those channels.
Disk-related channels use `2 * disk_radius_cm`; polar channels use
`2 * polar_outer_cm`; other material channels use
`2 * max(material_radius_cm, disk_radius_cm)`. An explicit positive reference
path can override this derivation. The target optical depth and target emission
are dimensionless totals across one reference path, not universal `1e-11`
coefficients.

Native configuration:

```text
stellarPhysicalChannel = rotational_fraction
stellarPhysicalScale = linear
stellarPhysicalRangeMin = 0
stellarPhysicalRangeMax = 1
stellarPhysicalOpticalProfile = material_support_v074
stellarPhysicalTargetOpticalDepth = 1
stellarPhysicalTargetEmission = 1
stellarPhysicalReferencePathCm = 0
stellarFeatureProfile = stellar_structures_v065
stellarReconstruction = voronoi
```

`0` selects the channel-derived reference path. The runtime emits
`STELLAR_PHYSICAL_OPTICAL_V074` with the resolved path and the
`zero_signal_transparent=true` contract. The legacy opacity/emission settings
remain accepted and active only under `legacy_v072`.

Focused tests:

```bash
c++ -std=c++11 -O2 -Isrc tests/test_stellar_physical_optical_v074.cpp \
  -o build/test_stellar_physical_optical_v074
build/test_stellar_physical_optical_v074
```

Image acceptance is deliberately separate from compilation. Run
`tools/audit_stellar_physical_visual_v074.py` on exactly 43 fixed-camera
WebGL/Voronoi pairs. The ledger binds identical camera hashes, linear `[0,1]`
rotational-fraction range, copper-blue palette, and the v074 profile. The audit
rejects per-pose white sheets and insufficient contrast/correlation and also
enforces aggregate luma, contrast, and structural thresholds. A checksum-only
or decode-only result is not acceptance.
