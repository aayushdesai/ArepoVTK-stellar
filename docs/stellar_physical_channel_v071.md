# Native physical channels v071

`stellarPhysicalChannel` is an opt-in transfer path inside the existing native
C++ Voronoi ray traversal. The default value, `optical`, preserves the current
stellar optical renderer.

The first release exposes the physical values already available at every
native interpolation sample:

- `density`, `temperature`, `speed`
- `radial_velocity`, `azimuthal_velocity`, `rotational_fraction`
- `angular_momentum_alignment`, `outward_axial_velocity`
- `outward_mass_flux_proxy`, `cylindrical_radius`, `axial_position`
- `entropy_proxy`, `magnetic_field_strength`

All non-optical channels use the same four-stop copper-blue palette. Configure
their transfer with:

```text
stellarPhysicalChannel = rotational_fraction
stellarPhysicalScale = linear
stellarPhysicalRangeMin = 0
stellarPhysicalRangeMax = 1
stellarPhysicalSymlogLinthresh = 0.01
stellarPhysicalOpacity = 1e-11
stellarPhysicalEmission = 1e-11
```

`stellarPhysicalRangeMin` and `stellarPhysicalRangeMax` are expressed after
the selected `linear`, `log10`, or `symlog` transform. Signed velocity channels
should normally use `symlog`; positive-definite channels can use `linear` or
`log10`.

The runtime marker is `STELLAR_PHYSICAL_CHANNEL_V071`. It records the channel,
scale, range, symmetric-log threshold, optical coefficients, palette, and
`traversal=native_voronoi`.

Pressure, sound speed, and directional magnetic-field channels are not part of
v071. They require preserving additional snapshot fields through the native
cell interpolation contract rather than reusing or relabeling legacy slots.
