#!/bin/bash
#SBATCH --job-name=arepovtk_stellar_p5
#SBATCH --account=caiazgrp
#SBATCH --partition=defaultp
#SBATCH --constraint=eta
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --threads-per-core=1
#SBATCH --mem=96G
#SBATCH --time=01:00:00
#SBATCH --output=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase5_validate_%j.out
#SBATCH --error=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase5_validate_%j.err

if [[ -z "${SLURM_JOB_ID:-}" ]]; then exit 2; fi
host="$(hostname -s)"
[[ "$host" == eta* ]] || exit 3
set -eo pipefail
environment_file="$(mktemp)"
trap 'rm -f "$environment_file"' EXIT
timeout 120s bash -c '
  set -eo pipefail
  source /nfs/scistore25/caiazgrp/adesai/Arepo/load_arepo_batch_env.sh
  [[ "${CONDA_DEFAULT_ENV:-}" == "Arepo_Env" ]]
  export -p
' >"$environment_file"
source "$environment_file"
rm -f "$environment_file"
trap - EXIT
set -u
[[ "${CONDA_DEFAULT_ENV:-}" == "Arepo_Env" ]] || exit 4

repo=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar
dependency=/nfs/scistore25/caiazgrp/adesai/Arepo/outreach_volume_movie/v031_rework/third_party/ArepoVTK_10b14c15/arepo
reference_commit=10b14c15fd864f01fcf1f3af893f8c740163f61f
arepo_commit=d924fb39f2933129614d9a36296099b8137b2cde
out="$repo/validation/phase5_${SLURM_JOB_ID}"
source_copy="$out/source"
[[ ! -e "$out" ]] || exit 5
[[ "$(git -C "$repo" rev-parse "$reference_commit")" == "$reference_commit" ]] || exit 6
[[ "$(git -C "$dependency" rev-parse HEAD)" == "$arepo_commit" ]] || exit 7
[[ -z "$(git -C "$dependency" status --porcelain)" ]] || exit 8
mkdir -p "$out" "$source_copy" "$out/artifacts/run1" "$out/artifacts/run2"

{
  echo "phase=5"
  echo "job_id=$SLURM_JOB_ID"
  echo "host=$host"
  echo "started=$(date -Is)"
  echo "branch=$(git -C "$repo" branch --show-current)"
  echo "candidate_commit=$(git -C "$repo" rev-parse HEAD)"
  echo "reference_commit=$reference_commit"
  echo "conda_env=$CONDA_DEFAULT_ENV"
  mpicxx --version | head -n 1
} >"$out/environment.txt"
git -C "$repo" status --porcelain=v1 >"$out/repo_status.txt"
git -C "$repo" diff --binary >"$out/candidate.diff"
git -C "$repo" diff --check >"$out/diff_check.txt"
git -C "$repo" ls-files --others --exclude-standard | sort \
  >"$out/untracked_files.txt"
while IFS= read -r file; do
  [[ -n "$file" ]] && sha256sum "$repo/$file"
done <"$out/untracked_files.txt" >"$out/untracked_files.sha256"
git -C "$repo" diff "$reference_commit" -- src/voronoi_3db.cpp src/voronoi_3db.h \
  >"$out/voronoi_reference.diff"
test ! -s "$out/voronoi_reference.diff"

rsync -a --exclude='.git/' --exclude='arepo/' --exclude='build/' \
  --exclude='validation/' "$repo/" "$source_copy/"
rsync -a --exclude='.git/' "$dependency/" "$source_copy/arepo/"
mkdir -p "$source_copy/build"
(cd "$source_copy" && find docs slurm src tests -type f -print0 | sort -z | \
  xargs -0 sha256sum) >"$out/source_manifest.sha256"
unset PYTHONPATH
export PYTHONNOUSERSITE=1
make -C "$source_copy" -j"$SLURM_CPUS_PER_TASK" >"$out/build.log" 2>&1
test -x "$source_copy/ArepoRT"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/tests/test_stellar_render_model_v051a.cpp" \
  -o "$source_copy/build/test_stellar_render_model_v051a" \
  >"$out/render_model_build.log" 2>&1
"$source_copy/build/test_stellar_render_model_v051a" >"$out/render_model_test.log" 2>&1
grep -q STELLAR_RENDER_MODEL_V051A_OK "$out/render_model_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/tests/test_stellar_render_model_v052a.cpp" \
  -o "$source_copy/build/test_stellar_render_model_v052a" \
  >"$out/render_model_v052_build.log" 2>&1
"$source_copy/build/test_stellar_render_model_v052a" \
  >"$out/render_model_v052_test.log" 2>&1
grep -q STELLAR_RENDER_MODEL_V052A_OK "$out/render_model_v052_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/tests/test_stellar_gpu_scene_format_v052.cpp" \
  -o "$source_copy/build/test_stellar_gpu_scene_format_v052" \
  >"$out/scene_format_build.log" 2>&1
"$source_copy/build/test_stellar_gpu_scene_format_v052" \
  >"$out/scene_format_test.log" 2>&1
grep -q STELLAR_GPU_SCENE_FORMAT_V052_OK "$out/scene_format_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/tests/test_stellar_scene_physics_v052.cpp" \
  -o "$source_copy/build/test_stellar_scene_physics_v052" \
  >"$out/scene_physics_build.log" 2>&1
"$source_copy/build/test_stellar_scene_physics_v052" \
  >"$out/scene_physics_test.log" 2>&1
grep -q STELLAR_SCENE_PHYSICS_V052_OK "$out/scene_physics_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/tests/test_stellar_reconstruction_v053.cpp" \
  -o "$source_copy/build/test_stellar_reconstruction_v053" \
  >"$out/reconstruction_build.log" 2>&1
"$source_copy/build/test_stellar_reconstruction_v053" \
  >"$out/reconstruction_test.log" 2>&1
grep -q STELLAR_RECONSTRUCTION_V053_OK "$out/reconstruction_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/tests/test_stellar_camera_v054.cpp" \
  -o "$source_copy/build/test_stellar_camera_v054" \
  >"$out/camera_build.log" 2>&1
"$source_copy/build/test_stellar_camera_v054" \
  >"$out/camera_test.log" 2>&1
grep -q STELLAR_CAMERA_V054_OK "$out/camera_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/src/stellar_camera_plan_v054.cpp" \
  -o "$source_copy/build/stellar_camera_plan_v054" \
  >"$out/camera_plan_build.log" 2>&1
"$source_copy/build/stellar_camera_plan_v054" \
  "$source_copy/tests/stellar_camera_landmarks_v054.tsv" \
  "$out/camera_path_run1.tsv" orbit 32 1200 \
  >"$out/camera_plan_run1.log" 2>&1
"$source_copy/build/stellar_camera_plan_v054" \
  "$source_copy/tests/stellar_camera_landmarks_v054.tsv" \
  "$out/camera_path_run2.tsv" orbit 32 1200 \
  >"$out/camera_plan_run2.log" 2>&1
cmp "$out/camera_path_run1.tsv" "$out/camera_path_run2.tsv"
if "$source_copy/build/stellar_camera_plan_v054" \
    "$source_copy/tests/stellar_camera_landmarks_v054.tsv" \
    "$out/camera_path_run1.tsv" orbit 32 1200 \
    >"$out/camera_plan_clobber.log" 2>&1; then
  echo "camera planner overwrote an existing path" >&2
  exit 10
fi
grep -q 'Cannot create camera path without overwrite' \
  "$out/camera_plan_clobber.log"
if "$source_copy/build/stellar_camera_plan_v054" \
    "$source_copy/tests/stellar_camera_landmarks_v054.tsv" \
    "$out/camera_path_invalid.tsv" orbit 32 invalid \
    >"$out/camera_plan_invalid.log" 2>&1; then
  echo "camera planner accepted an invalid orbit period" >&2
  exit 11
fi
grep -q 'Invalid orbit period' "$out/camera_plan_invalid.log"
test ! -e "$out/camera_path_invalid.tsv"
sha256sum "$out/camera_path_run1.tsv" "$out/camera_path_run2.tsv" \
  >"$out/camera_paths.sha256"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  "$source_copy/src/stellar_field_registry.cpp" \
  "$source_copy/tests/test_stellar_field_registry.cpp" \
  -o "$source_copy/build/test_stellar_field_registry" \
  >"$out/registry_build.log" 2>&1
"$source_copy/build/test_stellar_field_registry" >"$out/registry_test.log" 2>&1
grep -q 'STELLAR_FIELD_REGISTRY_TEST_OK fields=9' "$out/registry_test.log"

source /etc/profile.d/modules.sh
module load cuda/12.8.1
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -I"$source_copy/src" \
  "$source_copy/src/stellar_gpu_renderer_v051a.cu" \
  -o "$source_copy/build/stellar_gpu_renderer_v051a" \
  >"$out/gpu_renderer_build.log" 2>&1
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -I"$source_copy/src" \
  "$source_copy/src/stellar_gpu_renderer_v052a.cu" \
  -o "$source_copy/build/stellar_gpu_renderer_v052a" \
  >"$out/gpu_renderer_v052_build.log" 2>&1
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -I"$source_copy/src" \
  "$source_copy/src/stellar_gpu_renderer_v053a.cu" \
  -o "$source_copy/build/stellar_gpu_renderer_v053a" \
  >"$out/gpu_renderer_v053_build.log" 2>&1
[[ "${CONDA_DEFAULT_ENV:-}" == "Arepo_Env" ]] || exit 9

run_reference_suite() {
  local run_dir="$1"
  local log_dir="$2"
  local config output
  mkdir -p "$log_dir"
  for config in config_2.txt config_2b.txt; do
    output="$(awk '$1 == "imageFile" {print $3; exit}' "$source_copy/tests/$config")"
    rm -f "$source_copy/$output"
    (cd "$source_copy" && ./ArepoRT "tests/$config") >"$log_dir/${config%.txt}.log" 2>&1
    test -s "$source_copy/$output"
    cp "$source_copy/$output" "$run_dir/$output"
  done
}
run_reference_suite "$out/artifacts/run1" "$out/render_logs/run1"
run_reference_suite "$out/artifacts/run2" "$out/render_logs/run2"
(cd "$out/artifacts/run1" && sha256sum frame2.png frame2b.png) >"$out/render_run1.sha256"
(cd "$out/artifacts/run2" && sha256sum frame2.png frame2b.png) >"$out/render_run2.sha256"
diff -u "$out/render_run1.sha256" "$out/render_run2.sha256" >"$out/determinism.diff"

python - "$source_copy" "$out" <<'PY'
import json
import sys
from pathlib import Path
import numpy as np
from PIL import Image

source = Path(sys.argv[1])
out = Path(sys.argv[2])
results = {}
for name in ("frame2.png", "frame2b.png"):
    expected = np.asarray(Image.open(source / "tests" / name).convert("RGBA"))
    actual = np.asarray(Image.open(out / "artifacts" / "run1" / name).convert("RGBA"))
    delta = np.abs(expected.astype(np.int16) - actual.astype(np.int16))
    results[name] = {
        "exact": bool(np.array_equal(expected, actual)),
        "max_abs_channel_error": int(delta.max()),
        "rms_channel_error": float(np.sqrt(np.mean(delta.astype(np.float64) ** 2))),
    }
if not results["frame2.png"]["exact"]:
    raise SystemExit("required frame2.png reference changed")
(out / "pixel_validation.json").write_text(json.dumps(results, indent=2, sort_keys=True) + "\n")
PY

sha256sum "$source_copy/ArepoRT" \
  "$source_copy/build/test_stellar_render_model_v051a" \
  "$source_copy/build/test_stellar_render_model_v052a" \
  "$source_copy/build/test_stellar_gpu_scene_format_v052" \
  "$source_copy/build/test_stellar_scene_physics_v052" \
  "$source_copy/build/test_stellar_reconstruction_v053" \
  "$source_copy/build/test_stellar_camera_v054" \
  "$source_copy/build/stellar_camera_plan_v054" \
  "$source_copy/build/test_stellar_field_registry" \
  "$source_copy/build/stellar_gpu_renderer_v051a" \
  "$source_copy/build/stellar_gpu_renderer_v052a" \
  "$source_copy/build/stellar_gpu_renderer_v053a" >"$out/binaries.sha256"
{
  echo "status=PASS"
  echo "completed=$(date -Is)"
  echo "legacy_reference_exact=true"
  echo "deterministic=true"
  echo "stellar_render_model_v051_tests=1"
  echo "stellar_render_model_v052_tests=1"
  echo "separate_emissivity_extinction=true"
  echo "analytic_segment_integration=true"
  echo "stellar_scene_format_tests=1"
  echo "stellar_scene_physics_tests=1"
  echo "stellar_reconstruction_v053_tests=1"
  echo "runtime_reconstruction_modes=sph,idw,voronoi"
  echo "first_ring_support=true"
  echo "exact_center_guard=true"
  echo "normalized_aligned_fields=true"
  echo "deterministic_parent_fallback=true"
  echo "stellar_camera_v054_tests=1"
  echo "camera_axis_sign_continuity=true"
  echo "camera_axis_rate_bounded=true"
  echo "camera_center_acceleration_bounded=true"
  echo "camera_parallel_transport_roll=true"
  echo "camera_independent_extents=material,disk,outflow"
  echo "camera_path_deterministic=true"
  echo "camera_path_no_clobber=true"
  echo "camera_numeric_input_strict=true"
  echo "native_scene_exporter_build=true"
  echo "stellar_field_registry_tests=1"
  echo "gpu_renderer_v051_build=true"
  echo "gpu_renderer_v052_build=true"
  echo "gpu_renderer_v053_build=true"
  echo "gpu_renderer_executed=false"
  echo "voronoi_reference_modified=false"
} >"$out/summary.txt"
touch "$out/PHASE5_VALIDATE_OK"
cat "$out/summary.txt"
echo "PHASE5_VALIDATE_OK output=$out host=$host job=$SLURM_JOB_ID"
