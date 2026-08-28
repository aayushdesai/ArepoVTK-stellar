# Stellar interactive camera lab

## Purpose

The camera lab separates artistic camera discovery from snapshot rendering.
It consumes one existing portable v052 scene, deterministically selects a
feature-aware point cloud, and writes a self-contained WebGL page. The page can
be opened directly, so it does not require Mayavi, Matplotlib, a Python GUI, or
a development server.

The point cloud exposes density, temperature, speed, radial velocity,
rotational fraction, and signed outward axial velocity. These views are
diagnostic; the production renderer remains native C++ Voronoi.

## Build and inspect

Use the bundled workspace Python or another Python with NumPy:

```text
python3 tools/stellar_scene_camera_lab.py \
  --scene /path/to/existing_full_scene.bin \
  --scene-sha256 <accepted-scene-sha256> \
  --snapshot 721 \
  --max-points 140000 \
  --output /new/no-clobber/path/camera_lab_721.html
```

Optional `--center`, `--axis`, and `--display-radius-cm` values override the
deterministic density-center and angular-momentum-axis estimates. Production
authoring should use reviewed, hash-bound physical center and axis values.

Drag to orbit, Shift-drag or right-drag to pan, and use the wheel to change the
orthographic half extent. Enter a snapshot and add the current key pose. The
downloaded JSON uses `stellar_camera_keyframes_v001` and records position,
look-at, view direction, up, and screen half extent in cm.

## Compile a spline

At least two key poses must span the desired template path. The spline compiler
retains simulation time, center, physical axis, and material/disk/outflow
extents from an existing 21-column v055 template:

```text
python3 tools/stellar_camera_spline.py \
  --keyframes stellar_camera_keyframes.json \
  --template accepted_camera_path.tsv \
  --output candidate_spline_camera_path.tsv \
  --diagnostics candidate_spline_diagnostics.tsv
```

Look-at motion uses a restrained cubic Hermite spline, orientation uses
quaternion SQUAD, and orthographic scale uses a shape-preserving cubic spline
in log space. The output is no-clobber and directly parseable by the retained
v055 camera-path reader. Diagnostics report per-frame pan, zoom, and total
orientation change. This is an artistic review candidate, not a production
authorization: the accepted director and operations audit must still bind the
physical landmarks, enforce motion budgets, and prove behavior on evolving
snapshots.

Rebuild the camera lab with `--camera-path candidate_spline_camera_path.tsv` to
play the solved trajectory against the same 3D point cloud before authorizing
any native render.

## Operational boundary

This tool never reads an AREPO snapshot. Scene export, snapshot access, Slurm
submission, native rendering, and scientific audit remain operations-owned.
The intended first input is one already exported middle scene, preferably
snapshot 721. A single scene is sufficient to choose an orientation and inspect
physical channels, but not to prove scale or target tracking through time.
Additional reviewed poses should therefore be checked on a small set of
existing exported scenes before the spline enters any render pilot. A newly
exported scene is unnecessary when immutable full-cell v052 scenes already
exist.
