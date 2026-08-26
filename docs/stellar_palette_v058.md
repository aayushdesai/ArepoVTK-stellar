# Polar accent palette v058

`copper_blue_accent_v058` is a restrained successor to
`copper_blue_v057`. The v057 profile remains available unchanged for exact
reproduction of its accepted pilot products.

The v058 profile keeps the disk and dense remnant copper, brown, and gold. The
bulk of physically selected polar material uses a darker warm-brown ramp.
Cobalt is blended in only where the same polar material is simultaneously hot,
fast in the outward axial direction, and kinematically coherent. The profile
also lowers the composite polar weight relative to v057. It does not change
the density, temperature, geometric, velocity, or coherence rules that decide
whether a sample belongs to the wind.

Enable the profile with:

```text
stellarPaletteProfile = copper_blue_accent_v058
```

The `STELLAR_PALETTE_V058` provenance record includes the v058 base and accent
RGB endpoints, physical accent windows, layer weights, and overlap policy.
Release validation must retain exact legacy and v057 behavior, prove the warm
bulk-wind and blue hot/fast-wind regimes separately, reject unknown profiles,
and leave the native C++ Voronoi traversal unchanged.
