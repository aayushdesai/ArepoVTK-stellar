# Stellar production contract v055

## Native camera path

`stellar_camera_plan_v054` produces one strictly ordered, no-clobber camera
table from the full physical-landmark track. A native render opts in with:

```text
stellarCameraPath = /absolute/path/to/camera_path.tsv
```

`ArepoRT -s SNAPSHOT config.txt` uses `SNAPSHOT` as the exact table key. Before
camera construction, the matching row replaces camera position, look-at, up,
and orthographic `swScale`. Missing, duplicate, unordered, nonfinite,
nonorthogonal, or malformed rows are fatal. Camera tables and legacy `addKF`
entries are mutually exclusive. The existing keyframe parser remains available
only for legacy configurations.

The planner reads compact landmark tables and never reads AREPO snapshots. The
renderer still receives the per-snapshot stellar center, axis, bulk velocity,
disk radius, and polar classifier parameters from the validated feature track;
camera tables do not replace those physical transfer inputs.

## Campaign invariants

A production campaign freezes these values before launch:

- exact package commit and `ArepoRT` SHA-256;
- camera-path, feature-track, configuration-generator, and color-table hashes;
- reconstruction mode and its numeric parameters;
- transfer mode, emissivity, extinction, exposure, black point, and saturation;
- snapshot manifest, expected frame count, image dimensions, and frame rate;
- scheduler policy, eta guards, concurrency cap, and no-clobber output root.

Changing reconstruction, transfer, camera, or color creates a new campaign
namespace. Missing-frame recovery renders only absent tasks against the same
frozen manifest.

## Release gate

Promotion requires more than successful Slurm states:

- one uniquely named, decodable frame and checksum per expected snapshot;
- exact task-to-snapshot and camera-row coverage with no duplicates;
- finite frame metrics and recorded frame-source provenance;
- decodable movies and contact sheets with dimensions, duration, and checksums;
- comparison reel coverage when alternatives are evaluated;
- source, dependency, binary, camera, feature, config, and product manifests;
- explicit evidence that all AREPO reads, rendering, validation, and audit ran
  on `eta*` with `Arepo_Env`.

Operational Slurm launch, recovery, assembly, and audit scripts consume this
contract but remain separate from the renderer core.
