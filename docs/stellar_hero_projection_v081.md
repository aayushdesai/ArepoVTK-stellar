# Stellar adaptive hero projection v081

`hero_adaptive_copper_blue_v081` keeps the exact v080 Voronoi traversal and
physical sample definitions. Along each ray it retains the strongest supported
material cell and strongest coherent-outflow cell instead of adding every cell
through the full line of sight. This is a maximum-intensity diagnostic surface
projection, not radiative transfer: copper-blue is a diagnostic palette, and no
frequency-dependent opacity is available in the scene contract.

## Why v080 failed

The three v080 calibration poses were late epochs. Their material-column p90
was about `6e8` to `8e8 g cm^-2`; an early reviewed pose reached about `1e13`
at p90 and `1e14` at p95. One global reference therefore saturated early
epochs while still underexposing some late structure. More fundamentally, the
full-column sum blurred every supported foreground cell into a sheet. V080 also
passed its already-display-referred palette RGB through the legacy filmic and
power-law encoder, reducing saturation and creating pale beige-white images.

## v081 display contract

For each exact camera view, v053k measures the positive-ray p99 separately for
peak material density and peak coherent outward mass flux. The recorded
references are used only for display normalization. A fixed two-percent
response floor removes weak projected support. The reviewed pose opacity is the
display gain for both components. Camera-lab gamma, inversion, saturation,
brightness, range, and copper-blue styling remain bound per pose.

The normalized colors are written directly as display RGB over the same dark
background used by camera-lab. They do not pass through the legacy Arepo filmic
or `pow(1/2.3)` encoding. Hot support raises material intensity but never mixes
in a white or orange auxiliary hue: visible colors remain within the same
copper-blue palette used by camera-lab. The hero material mapping stops at the
saturated copper control point rather than entering the palette's pale cream
tail; hot white-dwarf support moves toward copper, while coherent outflow stays
on the blue control point.

This dominant-cell projection intentionally prioritizes sharp cinematic
morphology across a simulation whose density and flux scales evolve by orders
of magnitude. The selected physical values and exact p99 references remain in
every benchmark for scientific audit. The 24 diagnostic channel modes remain
on the retained v053h path and are unchanged.
