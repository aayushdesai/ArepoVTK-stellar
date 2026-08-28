# Layered structure-flux optical profile v070

`structure_flux_layered_v070` is an opt-in refinement of
`structure_flux_vivid_v068`. It addresses the Phase21 native-contact result in
which the disk rendered as a nearly uniform opaque beige foreground with a hard
visual boundary.

The profile keeps the v068 vivid polar colors, confidence response, mass-flux
proxy, emissivity, and extinction exactly unchanged. Only the disk response is
different:

- a substantially lower extinction floor and weaker density/rotation opacity
  terms move the selected v065 annulus away from the saturated source-function
  limit;
- a low emissivity floor with broader density response makes optically thin
  structure visible instead of giving every selected cell the same baseline;
- radial motion has a stronger copper-orange response, while predominantly
  rotating material retains a darker brown/gold mixture;
- the density texture window is shifted upward so low and intermediate density
  disk material does not immediately saturate the response.

The physical feature selector remains `stellar_structures_v065`; no cells are
reclassified. The portable scene schema and the density, temperature, position,
and velocity inputs are unchanged. The profile still requires composite
transfer and v065 features.

This is a visual-transfer candidate, not calibrated radiative transfer. It must
be compared against `structure_flux_vivid_v068` on matched preserved scenes.
The acceptance questions are internal disk contrast, annular/nonuniform
structure, white-dwarf visibility, absence of an artificial opacity edge, and
retention of the already accepted vivid polar appearance. Camera framing is a
separate decision.
