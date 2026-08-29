# Stellar physical optical transfer v076

`density_moment_v076` is an opt-in physical-channel transfer for native
Voronoi rays. It retains the v074/v075 profiles unchanged.

The profile separates three responsibilities:

- Density support determines where material contributes opacity and emission.
- The selected physical scalar determines hue and relative emissive strength.
- Copper-blue color is applied once after each complete ray, not once per cell.

The ray carries three scalar moments in the existing RGB accumulator:

1. emission-weighted styled scalar,
2. scalar weight,
3. emission-weighted intensity.

After integration, the renderer divides moment 1 by moment 2, applies the
copper-blue palette to that scalar, and multiplies by moment 3. This prevents
blue and copper cells on one line of sight from averaging into a beige sheet.
Zero rotational fraction no longer makes dense material disappear; it receives
the configured `stellarPhysicalEmissionSignalFloor`.

## Native configuration

```text
stellarPhysicalOpticalProfile = density_moment_v076
stellarPhysicalDensitySupportLog10Low = -9
stellarPhysicalDensitySupportLog10High = -5
stellarPhysicalEmissionSignalFloor = 0.25
stellarPhysicalColorGamma = 2.25
stellarPhysicalColorInvert = 0
stellarPhysicalTargetOpticalDepth = 0.2
stellarPhysicalTargetEmission = 1.0
stellarPhysicalReferencePathCm = 1e12
```

The density bounds are in `log10(g cm^-3)`. The reference path is explicit and
must be calibrated against representative rays; target optical depth and target
emission are dimensionless path-normalized controls. The example values are a
starting point, not an accepted visual profile.

The renderer emits `STELLAR_PHYSICAL_OPTICAL_V076` with the exact support,
moment, density, floor, gamma, brightness, and path-normalization settings.

## Focused acceptance

Use the exact 43 reviewed rotational-fraction cameras and the matching WebGL
controls. Evaluate several parameter waves from the same immutable v073 scenes.
Do not export or read raw snapshots again. Select a wave only if it restores:

- compact-object visibility in the opening,
- disk structure without a neutral bright sheet,
- blue polar support at late epochs,
- recognizable copper-blue separation across all reviewed cameras.

No movie should be rendered until one still-image wave passes that review.
