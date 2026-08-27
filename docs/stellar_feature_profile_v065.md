# Stellar structure selector profile v065

Phase 15 showed that the legacy disk selector is not a circumstellar-disk
selector in the production merger. Across four disk epochs and five weight
thresholds, 86.1--98.3% of selected weight overlaps the merger component,
95.2--99.8% lies inside 0.35 disk radii, and 82.6--92.7% is at
`log10(rho) >= 2`. Its weighted median radius is only 0.017--0.044 disk radii.

The opt-in `stellar_structures_v065` profile changes feature weights, not
Voronoi reconstruction, physical fields, palette, camera, or optical
integration. `legacy_v064` remains the default and exact reference.

## Disk

The legacy disk weight is multiplied by two explicit retention terms:

- an annular gate rising from zero at 0.12 disk radii to one at 0.32 disk
  radii;
- a density retention taper from one at `log10(rho)=2.5` to a floor of 0.08
  at `log10(rho)=4.5`.

This targets rotational material outside the compact remnant while retaining a
small, documented contribution from dense disk structures. It does not infer a
Keplerian orbit or claim that the selected gas is a settled disk.

## Polar wind

The legacy polar weight is retained, but low-confidence support is attenuated.
The multiplier rises from 0.20 to one as the legacy polar weight moves from
0.08 to 0.28. This keeps a faint wind envelope while making strong coherent
outflow support dominate framing and composite emission.

## Release policy

The profile is selected by `stellarFeatureProfile`. Native logs record the
exact name, id, and constants. Packed-scene diagnostics accept the same named
profile. GPU renderers remain legacy-profile build contracts until separate
runtime parity validation. A profile is not promoted from unit fixtures: the
ten preserved Phase 15 scenes must first establish disk/core separation and
signed-lobe support, followed by bounded native-Voronoi contacts.
