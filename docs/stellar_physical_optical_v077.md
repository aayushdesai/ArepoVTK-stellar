# Stellar physical optical profile v077

`normalized_moment_v077` retains the v076 density-supported Voronoi transfer
and fixes its post-ray display normalization. V076 stored weighted scalar,
weight, and weighted amplitude moments, but exposed the weighted amplitude
line integral directly as intensity. For an optically thick ray that value
approaches `target_emission / target_optical_depth`, which drove low-optical-
depth renders into white clipping.

V077 decodes the same moments as:

- scalar hue: `weighted_scalar / weight`;
- mean emissive amplitude: `weighted_amplitude / weight`;
- emitted coverage: `min(weight * target_optical_depth, target_emission)`;
- linear intensity: `emitted_coverage * mean_emissive_amplitude`.

The result is bounded by `target_emission`, retains copper-blue hue until the
display transform, and still responds monotonically to supported path length.
All v074-v076 profiles retain their exact previous behavior.

Configuration uses the existing density-moment parameters with:

```text
stellarPhysicalOpticalProfile = normalized_moment_v077
```

The reviewed 43-pose rotational-fraction set is the visual acceptance surface.
A movie is not an acceptance prerequisite.
