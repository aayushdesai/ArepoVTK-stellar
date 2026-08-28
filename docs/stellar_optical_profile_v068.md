# Structure-flux optical profiles v068

Phase 20 adds two opt-in native optical profiles without changing the portable
scene schema or the physical feature selectors:

- `structure_flux_balanced_v068`
- `structure_flux_vivid_v068`

Both profiles require `stellarTransferMode = composite` and
`stellarFeatureProfile = stellar_structures_v065`. Existing
`legacy_v052`, `copper_blue_v057`, and `copper_blue_accent_v058` profiles
retain their original calculation path and output contract.

## Physical channels

The profiles use fields already stored by scene format v052: density,
temperature, position, and velocity. The validated v065 selector still defines
merger, annular disk, and polar material. The optical model adds only bounded,
dimensionless response terms:

- Disk emissivity and opacity vary with log density, rotational support, and
  radial-to-total speed. Radial streams shift toward copper-orange while
  rotating material retains the gold/thermal mixture. Reduced and structured
  disk opacity is intended to expose internal annular structure rather than a
  uniform opaque foreground.
- Polar emissivity varies with a normalized mass-flux proxy: density support
  times outward axial-speed support times directional coherence. This proxy is
  not a calibrated physical mass flux. It brightens coherent, mass-carrying
  wind and shifts it toward cobalt while retaining a warm low-confidence bulk
  component.
- Merger emissivity retains the thermal color model with a modest radial-motion
  response to distinguish tails and streams.

No composition, ionization, metallicity, magnetic-field, pressure, Mach,
radiation-field, or particle-ID color interpretation is implied.

## Provenance and validation

Native runs emit `STELLAR_OPTICAL_PROFILE_V068` with the exact profile,
feature profile, color constants, emissivity/extinction scales, response
windows, composite weights, and proxy definition. The profile ID is carried in
the existing palette-profile field, so `StellarTransferParameters` and packed
scene serialization remain unchanged.

The two variants are candidates for matched fixed-camera contacts. Neither is
promoted until native Voronoi contacts demonstrate white-dwarf visibility,
nonuniform annular disk structure without an artificial cutoff, brighter
colorful wind, and acceptable edge contact.
