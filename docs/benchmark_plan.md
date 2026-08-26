# Stellar renderer benchmark plan

## Objective

Minimize complete movie campaign wall time while preserving reference image
quality and deterministic scientific provenance. A faster isolated kernel does
not qualify if snapshot staging, scene setup, queueing, or assembly dominates
the campaign.

## Correctness gates

A candidate implementation must preserve:

- the native AREPO Voronoi traversal reference output;
- field values and unit transformations on analytic fixtures;
- ray entry, cell sequence, segment length, and accumulated optical depth;
- deterministic output for a frozen compiler, dependency, and thread count;
- frame coverage and no-clobber provenance.

Image comparisons will report exact pixel equality where expected, maximum and
RMS channel error, structural similarity, non-black coverage, and percentile
differences. Scientific comparisons will report field-integral and landmark
errors independently of final tone mapping.

## Timing decomposition

Record wall and CPU time for:

1. snapshot open and field reads;
2. AREPO initialization and tessellation;
3. connectivity and auxiliary structures;
4. camera and landmark calculation;
5. ray traversal and sampling;
6. transfer-function evaluation;
7. image encoding and output;
8. frame assembly and campaign makespan.

Also record peak RSS, bytes read and written, cells, rays, samples, mean and
tail cells per ray, thread count, host, compiler, module stack, and source and
binary hashes.

## Benchmark matrix

Use three data tiers:

- analytic tiny grids for correctness and continuous integration;
- one early, one merger, and one outflow stellar snapshot for representative
  behavior;
- a short contiguous sequence for camera and campaign tests.

Use fixed 512x512, 1920x1080, and 3840x2160 frames. Sweep eta thread counts
within one node. GPU candidates are compared only after the CPU reference is
frozen and must include transfer overhead and scene preparation.

## Acceptance

Each optimization records a before/after manifest. It is accepted only if:

- every correctness gate passes;
- no required provenance field is missing;
- median and worst-case times are measured over repeated runs;
- campaign makespan improves under the current availability-first scheduler;
- memory and I/O remain within declared limits.

## Initial Phase 0 baseline

`slurm/phase0_validate.sbatch` creates a job-specific validation directory,
records tracked-source and reference-image hashes, builds the unmodified
renderer with pinned public AREPO, runs upstream tiny-grid tests twice, checks
determinism, compares committed reference pixels, and runs the standalone
stellar field-registry unit test.

## Development phases

The package roadmap is recorded in `docs/stellar_renderer_roadmap.md`. Every
phase keeps the previous model and focused tests as fixtures, validates in an
eta-only job-specific source copy, and records a zero diff for the native
Voronoi traversal until the reconstruction phase has independent scientific
and image-quality acceptance tests.
