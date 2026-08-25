# Stellar visualization Phase 0 architecture

## Immutable reference

Phase 0 starts from ArepoVTK commit
`10b14c15fd864f01fcf1f3af893f8c740163f61f`. The public AREPO dependency
used by the validation job is commit
`d924fb39f2933129614d9a36296099b8137b2cde`.

The existing C++ Voronoi traversal remains the correctness reference. Phase 0
does not modify:

- `src/arepo.cpp`
- `src/arepoInterp.cpp`
- `src/integrator.cpp`
- `src/voronoi_3db.cpp`

## Current architecture

1. `fileio.cpp` parses a text configuration and collects transfer functions
   and camera keyframes.
2. `snapio.cpp` reads selected AREPO fields into `P` and `SphP`. Several
   unrelated structure members are reused as visualization storage.
3. `arepo.cpp` constructs the AREPO mesh, evaluates fields, and advances rays
   through Voronoi cells. `arepoTree.cpp` provides the alternative tree
   sampler.
4. `transfer.cpp` maps field names to nine fixed numeric slots and converts
   samples into emission. Absorption is density based.
5. `camera.cpp`, `keyframe.cpp`, and `renderer.cpp` generate rays, animate
   configured camera parameters, and schedule pthread render tasks.
6. `fileio_img.cpp` writes PNG or TGA products.

## Phase 0 finding

The primary extension boundary is the field path, not the Voronoi traversal.
Field names, snapshot datasets, derived quantities, units, and transfer slots
are currently coupled across `snapio.cpp`, `arepo.cpp`, `transfer.h`, and
`transfer.cpp`. A stellar renderer needs this contract to be explicit before
new physics fields are added.

`stellar_field_registry` is initially a compatibility registry for the nine
legacy slots. It records canonical names, aliases, source datasets, source
kind, preferred display scale, and whether a field is a high-priority stellar
quantity. It is intentionally not wired into production rendering in Phase 0;
that integration must be a separately validated change.

## Validation layers

Every development stage must pass these layers in order:

1. Registry and configuration unit tests without AREPO or HDF5.
2. Upstream small-grid renders using `tests/grid_2.hdf5` and
   `tests/grid_2b.hdf5`.
3. Exact repeat-run hashes for deterministic reference output.
4. Pixel comparison against the committed upstream unit images.
5. A small stellar snapshot schema probe and one-frame render on eta.
6. Side-by-side reference and candidate renders with quantitative image and
   field-integral comparisons.
7. Performance benchmarks only after correctness gates pass.

`tests/config_3.txt` is not an upstream reference test: its camera view axis is
parallel to `cameraUp`, so `LookAt()` correctly rejects the degenerate basis,
and the repository contains no committed `frame3.png`. Phase 0 records this
fixture as invalid rather than weakening the camera guard.

All builds, tests, snapshot reads, renders, and validation run through Slurm on
eta nodes. Login-node work is limited to source, git, scheduler, and compact
result inspection.

## Stellar roadmap

The next field-registry milestone should inspect actual merger snapshot schemas
before assigning dataset names or scientific definitions. Candidate families
to evaluate are:

- thermodynamic state and composition;
- velocity in center-of-mass, radial, cylindrical, and corotating frames;
- angular momentum, Bernoulli-like unbound diagnostics, and Mach number;
- magnetic pressure, plasma beta, and field geometry;
- nuclear energy generation and burning-state tracers;
- provenance for units, transforms, clipping, and transfer-function bounds.

No candidate is promoted until its definition, units, required datasets, and
validation test are recorded.

Camera work should be driven by physical landmarks rather than snapshot box
coordinates. The intended progression is:

- robust center and scale estimates;
- orbital-plane and remnant-axis tracking with sign continuity;
- separate disk, polar-outflow, and unbound-tail landmarks;
- temporally smoothed camera position, look-at, roll, and field of view;
- shot-level constraints on angular velocity, acceleration, and framing.

## Dependency boundary

The upstream repository ignores `arepo/`, `libpng/`, and `build/`. The Phase 0
job stages a clean source copy and a clean public AREPO dependency copy in a
job-specific validation directory. It never builds inside the immutable
upstream reference tree and never reads production movie products.
