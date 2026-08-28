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

V072 extends the same opt-in interface for native Voronoi reconstruction with
the snapshot's full magnetic vector, gas pressure, and sound speed. It adds
`magnetic_field_axial`, `magnetic_field_azimuthal`, `magnetic_pressure`,
`alfven_speed`, `field_velocity_alignment`, `toroidal_field_fraction`,
`poloidal_field_fraction`, `plasma_beta`, `gas_pressure`, `sound_speed`, and
`mach_number`. It also evaluates `magnetic_field_strength` in Gauss and
`entropy_proxy` as `P/rho^(5/3)` from the preserved physical fields. Its marker
is `STELLAR_PHYSICAL_CHANNEL_V072`.
