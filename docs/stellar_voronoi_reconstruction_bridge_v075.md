# Point-cloud to Voronoi rendering bridge

The camera lab and ArepoRT answer different questions and should share intent,
not pretend to be pixel-identical renderers.

- Camera-lab WebGL draws sampled cell centers with screen-space point size and
  alpha blending. It is an interactive structure and camera editor.
- ArepoRT traverses the exact AREPO Voronoi tessellation. Every ray segment is
  integrated through the owning cell using its physical path length. It is the
  production volume renderer.

The bridge is therefore the immutable camera, physical scalar definition,
scale/range, palette, scalar gamma/inversion, saturation, brightness, scene
hash, and style-binding provenance. WebGL point size, point opacity, and point
budget remain explicitly backend-specific.

## Why the old result became a white sheet

The legacy physical transfer assigned at least eight percent opacity and
emission to every finite scalar value. Rotational fraction is finite at zero,
so long rays through millions of cells accumulated to an opaque, pale sheet.
This was not a Voronoi face-intersection failure. V074 removed the floor and
added feature support, but still multiplied both opacity and emission by the
same scalar amplitude.

`separated_support_v075` makes the roles explicit. The scalar controls color
and emission. The validated v065 material class controls optical support. A
small relevance ramp makes zero signal transparent without letting numerical
noise switch on full opacity. Channel-specific reference lengths keep the
same coefficient from being applied indiscriminately to disks and outflows.

## What remains piecewise constant

The exact Voronoi mode currently assigns each cell-centered field throughout
that cell. Visible cell boundaries can therefore be scientifically honest but
visually abrupt. Smoothing them correctly requires a new, opt-in field
reconstruction contract, not a display adjustment.

The preferred future reconstruction is a monotonic linear field inside each
cell using native AREPO least-squares gradients, with a neighbor min/max
limiter and face-continuity diagnostics. The v073 scene already carries
neighbor connectivity but not accepted field gradients. Estimating gradients
from exported neighbors is possible, but must be versioned and compared with
native gradients before production use. Until then, v075 deliberately leaves
the piecewise-constant tessellation unchanged.
