# Stellar palette profiles v057

The stellar transfer model exposes a versioned `stellarPaletteProfile` setting.
The profile changes only layer colors and composite weights; merger, disk, and
polar material selection remains controlled by the existing physical density,
temperature, velocity, coherence, and geometric gates.

## Profiles

- `legacy_v052` is the default. It preserves the Phase 7 transfer constants and
  reference-image behavior for configurations that do not specify a profile.
- `copper_blue_v057` uses copper, brown, and gold for dense rotating material
  and a restrained cobalt-blue ramp for coherent outward polar material. At
  disk/outflow overlap, the lesser red/blue contribution is bridged through
  green to produce a neutral boundary instead of a violet or magenta fringe.

Use the new profile in a native configuration with:

```text
stellarPaletteProfile = copper_blue_v057
```

The renderer emits a `STELLAR_PALETTE_V057` provenance line containing the
profile name, numeric profile id, RGB endpoints, temperature mixes, composite
weights, and overlap policy. Unknown profile names are fatal.

## Validation contract

Release validation must prove that the default profile retains exact legacy
reference images, the copper disk is red-dominant, the polar layer is strongly
blue-dominant, composite red/blue overlap is neutralized, invalid profile names
are rejected, and the current C++ Voronoi traversal is unchanged.

GPU production remains gated separately. Existing v053 GPU manifests select
the legacy profile; a later versioned manifest extension is required before a
non-legacy profile can be scheduled on a GPU.
