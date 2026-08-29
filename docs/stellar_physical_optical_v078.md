# Stellar physical optical profile v078

`composite_moment_v078` retains the v077 normalized ray moments and replaces
the spatially unbounded density-only support with one bounded stellar-structure
support:

```text
feature_support = 1 - (1 - merger) * (1 - disk) * (1 - polar)
support = density_support * feature_support
```

The merger, disk, and polar weights are the existing
`stellar_structures_v065` quantities. The union introduces no new tunable
parameters: any recognized stellar structure may contribute, while unrelated
box material is transparent. These weights are recomputed per cell from each
snapshot's position, density, temperature, and velocity; they are not stored
epoch labels. Scalar hue, mean emissive amplitude, and emitted coverage use the
v077 normalized post-ray decoder.

Configuration uses the existing density-moment parameters with:

```text
stellarPhysicalOpticalProfile = composite_moment_v078
```

All v072-v077 profiles retain their exact previous behavior. Acceptance is one
matched 43-pose rotational-fraction still set; no movie or other channel is
part of this profile handoff.
