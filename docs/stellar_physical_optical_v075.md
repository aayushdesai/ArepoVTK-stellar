# Separated physical support v075

`separated_support_v075` is an opt-in physical-channel transfer for native
Voronoi rendering. It leaves `legacy_v072` and `material_support_v074`
unchanged.

The native ray marcher still traverses the exact AREPO Voronoi tessellation and
integrates each cell segment with its physical path length. V075 changes only
the optical interpretation of a cell:

- the selected scalar sets copper-blue color and emitted intensity;
- the v065 merger/disk/polar feature weight sets physical optical support;
- a short scalar relevance ramp removes opacity at exactly zero and prevents a
  tiny nonzero scalar from immediately making the whole supported volume
  opaque;
- extinction and emission are normalized by a channel-specific reference path;
- scalar gamma/inversion are applied to color only, not to physical support;
- final brightness and saturation are explicit display operations.

For rotational fraction this means non-rotating or non-disk cells contribute
neither emission nor extinction. Once rotation is relevant, the validated v065
disk support controls extinction while the rotational fraction controls
emission and hue. This avoids both the historical positive opacity floor and
the v074 coupling in which the scalar amplitude multiplied opacity everywhere.

```text
stellarPhysicalOpticalProfile = separated_support_v075
stellarPhysicalTargetOpticalDepth = 1
stellarPhysicalTargetEmission = 1
stellarPhysicalReferencePathCm = 0
stellarPhysicalOpacitySignalThreshold = 0.02
stellarPhysicalColorGamma = 2.25
stellarPhysicalColorInvert = 0
stellarSaturation = 0.8
stellarDisplayBrightness = 1.05
```

A zero reference path selects the existing channel-specific physical scale.
The camera-lab `compile-render-intent` command writes these values from a
reviewed v002 pose/style bundle. WebGL point size and point opacity remain
backend-specific and are not reinterpreted as volume extinction.

This profile does not smooth, resample, or interpolate cells. A future
gradient-limited reconstruction would require a separately versioned field
gradient contract and visual/scientific review.
