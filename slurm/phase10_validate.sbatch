#!/bin/bash
#SBATCH --job-name=arepovtk_stellar_p10
#SBATCH --account=caiazgrp
#SBATCH --partition=defaultp
#SBATCH --constraint=eta
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --threads-per-core=1
#SBATCH --mem=96G
#SBATCH --time=01:00:00
#SBATCH --output=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase10_validate_%j.out
#SBATCH --error=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase10_validate_%j.err

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
expected_branch=stellar/phase10-transition-windows-20260826T193336Z
expected_base=659f5238e4c5fbdd629aaeef015ff15b88903c1b
out="$repo/validation/phase10_${SLURM_JOB_ID}"
source_copy="$out/source"
[[ ! -e "$out" ]] || exit 5
[[ "$(git -C "$repo" rev-parse "$reference_commit")" == "$reference_commit" ]] || exit 6
[[ "$(git -C "$dependency" rev-parse HEAD)" == "$arepo_commit" ]] || exit 7
[[ -z "$(git -C "$dependency" status --porcelain)" ]] || exit 8
mkdir -p "$out" "$source_copy" "$out/artifacts/run1" "$out/artifacts/run2"

{
  echo "phase=10"
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

test "$(git -C "$repo" branch --show-current)" = "$expected_branch"
test "$(git -C "$repo" rev-parse HEAD)" = "$expected_base"
printf '%s\n' \
  ' M Makefile' \
  ' M src/keyframe.cpp' \
  '?? docs/stellar_cinematic_director_v059.md' \
  '?? slurm/phase10_validate.sbatch' \
  '?? src/stellar_camera_director_v059.cpp' \
  '?? src/stellar_cinematic_director_v059.cpp' \
  '?? src/stellar_cinematic_director_v059.h' \
  '?? tests/test_stellar_cinematic_director_v059.cpp' \
  >"$out/expected_repo_status.txt"
cmp "$out/expected_repo_status.txt" "$out/repo_status.txt"
test ! -s "$out/diff_check.txt"
printf '%s\n' \
  'docs/stellar_cinematic_director_v059.md' \
  'slurm/phase10_validate.sbatch' \
  'src/stellar_camera_director_v059.cpp' \
  'src/stellar_cinematic_director_v059.cpp' \
  'src/stellar_cinematic_director_v059.h' \
  'tests/test_stellar_cinematic_director_v059.cpp' \
  >"$out/expected_untracked_files.txt"
cmp "$out/expected_untracked_files.txt" "$out/untracked_files.txt"
test "$(sha256sum "$out/candidate.diff" | awk '{print $1}')" = \
  b4979467e966abf5d261161e204fb98fa43938293616a2191affcd19532794f6
sha256sum "$repo/slurm/phase10_validate.sbatch" >"$out/validator.sha256"
(
  cd "$repo"
  printf '%s  %s\n' \
    a10b73b7322d4548284e8c9496a33c13631b447bb06b850e7fef145217139559 Makefile \
    973f4d1e6ce1e8695992863042eec8da52411fb7596c08b9811fe85466f20e3e src/keyframe.cpp \
    e8946226bd32ad8201e4dfc2db6ce288c33cd44424cc9adfcaa226e7036aa9ff docs/stellar_cinematic_director_v059.md \
    4f11edd18d385ac9df69319cf1afa0c512561e3f8f7e98926c4844d5aafc7f7c src/stellar_camera_director_v059.cpp \
    da379539f9fe56bb2d4f6517f5faf3fab56886dfbcb5ddac9db3051565015969 src/stellar_cinematic_director_v059.cpp \
    f9c19763ad2e5477cb138dc661e925798fc6134119f143d62e2112712fc2d57b src/stellar_cinematic_director_v059.h \
    2d45d77296fd38b945b17bf754e7b9f6eb9fb408fa193617655b9370d7d5d360 tests/test_stellar_cinematic_director_v059.cpp \
    | tee "$out/expected_candidate_files.sha256" \
    | sha256sum -c -
) >"$out/candidate_hash_validation.log" 2>&1

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
make -C "$source_copy" stellar_camera_director_v056 \
  >"$out/cinematic_director_make.log" 2>&1
test -x "$source_copy/stellar_camera_director_v056"

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

make -C "$source_copy" test_stellar_palette_v057 \
  >"$out/palette_v057_build.log" 2>&1
test -x "$source_copy/test_stellar_palette_v057"
"$source_copy/test_stellar_palette_v057" \
  >"$out/palette_v057_test.log" 2>&1
grep -q '^STELLAR_PALETTE_V057_OK profile=copper_blue_v057$' \
  "$out/palette_v057_test.log"

make -C "$source_copy" test_stellar_palette_v058 \
  >"$out/palette_v058_build.log" 2>&1
test -x "$source_copy/test_stellar_palette_v058"
"$source_copy/test_stellar_palette_v058" \
  >"$out/palette_v058_test.log" 2>&1
grep -q '^STELLAR_PALETTE_V058_OK profile=copper_blue_accent_v058$' \
  "$out/palette_v058_test.log"

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

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/src/stellar_camera_path_v055.cpp" \
  "$source_copy/tests/test_stellar_camera_path_v055.cpp" \
  -o "$source_copy/build/test_stellar_camera_path_v055" \
  >"$out/camera_path_build.log" 2>&1
"$source_copy/build/test_stellar_camera_path_v055" \
  "$out/camera_path_run1.tsv" >"$out/camera_path_test.log" 2>&1
grep -q STELLAR_CAMERA_PATH_V055_OK "$out/camera_path_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/src/stellar_cinematic_director_v056.cpp" \
  "$source_copy/src/stellar_camera_path_v055.cpp" \
  "$source_copy/tests/test_stellar_cinematic_director_v056.cpp" \
  -o "$source_copy/build/test_stellar_cinematic_director_v056" \
  >"$out/cinematic_director_build.log" 2>&1
"$source_copy/build/test_stellar_cinematic_director_v056" \
  >"$out/cinematic_director_test.log" 2>&1
grep -q STELLAR_CINEMATIC_DIRECTOR_V056_OK \
  "$out/cinematic_director_test.log"

c++ -std=c++11 -Wall -Wextra -Werror -pedantic -I"$source_copy/src" \
  "$source_copy/src/stellar_cinematic_director_v056.cpp" \
  "$source_copy/src/stellar_camera_director_v056.cpp" \
  -o "$source_copy/build/stellar_camera_director_v056" \
  >"$out/cinematic_director_cli_build.log" 2>&1

run_director() {
  local suffix="$1"
  "$source_copy/build/stellar_camera_director_v056" \
    --landmarks "$source_copy/tests/stellar_camera_landmarks_v054.tsv" \
    --shots "$source_copy/tests/stellar_cinematic_shots_v056.tsv" \
    --path "$out/cinematic_path_${suffix}.tsv" \
    --diagnostics "$out/cinematic_diagnostics_${suffix}.tsv" \
    --manifest "$out/cinematic_manifest_${suffix}.txt" \
    --preview "$out/cinematic_preview_${suffix}.svg" \
    --center-response-seconds 80 \
    --axis-max-rate-deg-per-s 0.17188733853924698 \
    >"$out/cinematic_director_${suffix}.log" 2>&1
  grep -q 'STELLAR_CAMERA_DIRECTOR_V056_OK rows=4 shots=2 dry_run=false' \
    "$out/cinematic_director_${suffix}.log"
}
run_director run1
run_director run2
cmp "$out/cinematic_path_run1.tsv" "$out/cinematic_path_run2.tsv"
cmp "$out/cinematic_diagnostics_run1.tsv" \
  "$out/cinematic_diagnostics_run2.tsv"
cmp "$out/cinematic_preview_run1.svg" "$out/cinematic_preview_run2.svg"
grep -v '_output=' "$out/cinematic_manifest_run1.txt" \
  >"$out/cinematic_manifest_run1.normalized"
grep -v '_output=' "$out/cinematic_manifest_run2.txt" \
  >"$out/cinematic_manifest_run2.normalized"
cmp "$out/cinematic_manifest_run1.normalized" \
  "$out/cinematic_manifest_run2.normalized"
grep -q '^schema=stellar_cinematic_direction_manifest_v056$' \
  "$out/cinematic_manifest_run1.txt"
grep -q '^shot.1.name=positive_outflow$' \
  "$out/cinematic_manifest_run1.txt"
grep -q '^shot.1.lobe_sign=1$' "$out/cinematic_manifest_run1.txt"
grep -q '^shot.1.orbit_period_seconds=0$' \
  "$out/cinematic_manifest_run1.txt"
grep -q 'Stellar cinematic camera dry-run' \
  "$out/cinematic_preview_run1.svg"
grep -q '^# schema=stellar_camera_diagnostics_v056$' \
  "$out/cinematic_diagnostics_run1.tsv"
"$source_copy/build/test_stellar_camera_path_v055" \
  "$out/cinematic_path_run1.tsv" \
  >"$out/cinematic_native_path_test.log" 2>&1
grep -q STELLAR_CAMERA_PATH_V055_OK "$out/cinematic_native_path_test.log"

"$source_copy/build/stellar_camera_director_v056" \
  --landmarks "$source_copy/tests/stellar_camera_landmarks_v054.tsv" \
  --shots "$source_copy/tests/stellar_cinematic_shots_v056.tsv" \
  --dry-run \
  --diagnostics "$out/cinematic_diagnostics_dry.tsv" \
  --manifest "$out/cinematic_manifest_dry.txt" \
  --preview "$out/cinematic_preview_dry.svg" \
  >"$out/cinematic_director_dry.log" 2>&1
grep -q 'STELLAR_CAMERA_DIRECTOR_V056_OK rows=4 shots=2 dry_run=true' \
  "$out/cinematic_director_dry.log"
test ! -e "$out/cinematic_path_dry.tsv"
test -s "$out/cinematic_diagnostics_dry.tsv"
test -s "$out/cinematic_manifest_dry.txt"
test -s "$out/cinematic_preview_dry.svg"
grep -q '^dry_run=true$' "$out/cinematic_manifest_dry.txt"

sha256sum "$out/cinematic_path_run1.tsv" \
  "$out/cinematic_diagnostics_run1.tsv" \
  "$out/cinematic_manifest_run1.txt" \
  "$out/cinematic_preview_run1.svg" \
  >"$out/cinematic_before_clobber.sha256"
if run_director run1 >"$out/cinematic_no_clobber_outer.log" 2>&1; then
  echo "cinematic director overwrote an existing product" >&2
  exit 12
fi
grep -q 'Refusing to overwrite' "$out/cinematic_director_run1.log"
sha256sum "$out/cinematic_path_run1.tsv" \
  "$out/cinematic_diagnostics_run1.tsv" \
  "$out/cinematic_manifest_run1.txt" \
  "$out/cinematic_preview_run1.svg" \
  >"$out/cinematic_after_clobber.sha256"
cmp "$out/cinematic_before_clobber.sha256" \
  "$out/cinematic_after_clobber.sha256"
if "$source_copy/build/stellar_camera_director_v056" positional \
    >"$out/cinematic_positional.log" 2>&1; then
  echo "cinematic director accepted a positional argument" >&2
  exit 13
fi
grep -q 'Unknown or positional option' "$out/cinematic_positional.log"
if "$source_copy/build/stellar_camera_director_v056" \
    --landmarks "$source_copy/tests/stellar_camera_landmarks_v054.tsv" \
    --shots "$source_copy/tests/stellar_cinematic_shots_v056.tsv" \
    --dry-run \
    --diagnostics "$out/invalid_numeric_diagnostics.tsv" \
    --manifest "$out/invalid_numeric_manifest.txt" \
    --preview "$out/invalid_numeric_preview.svg" \
    --center-response-seconds invalid \
    >"$out/cinematic_invalid_numeric.log" 2>&1; then
  echo "cinematic director accepted invalid numeric input" >&2
  exit 14
fi
grep -q 'Invalid --center-response-seconds' \
  "$out/cinematic_invalid_numeric.log"
test ! -e "$out/invalid_numeric_diagnostics.tsv"
test ! -e "$out/invalid_numeric_manifest.txt"
test ! -e "$out/invalid_numeric_preview.svg"

make -C "$source_copy" stellar_camera_director_v059 \
  test_stellar_cinematic_director_v059 \
  >"$out/cinematic_v059_make.log" 2>&1
test -x "$source_copy/stellar_camera_director_v059"
test -x "$source_copy/test_stellar_cinematic_director_v059"
"$source_copy/test_stellar_cinematic_director_v059" \
  >"$out/cinematic_v059_test.log" 2>&1
grep -q '^STELLAR_CINEMATIC_DIRECTOR_V059_OK$' \
  "$out/cinematic_v059_test.log"

cat >"$out/v059_landmarks.tsv" <<'EOF_V059_LANDMARKS'
# snapshot time cx cy cz ax ay az material disk polar
100 0 0 0 0 0 0 1 10 8 12
101 10 1 0 0 0.02 0 0.9998 11 8.5 13
102 20 2 0 0 0.04 0.01 0.9991 12 9 15
103 30 3 1 0 0.06 0.01 0.9981 13 10 18
104 40 4 2 0 0.08 0.02 0.9966 14 11 22
105 50 5 3 1 0.10 0.02 0.9948 15 12 28
106 60 6 4 2 0.12 0.03 0.9923 16 13 35
EOF_V059_LANDMARKS
cat >"$out/v059_shots.tsv" <<'EOF_V059_SHOTS'
# schema=stellar_cinematic_shots_v059
approach 100 102 orbit binary 10 20 1.10 30 - 1 - linear
disk_reveal 103 104 disk_oblique disk 35 38 1.06 - - 1 104 smoothstep
north_lobe 105 106 outflow_follow positive_lobe 45 8 1.12 - - 1 106 smootherstep
EOF_V059_SHOTS

run_v059_loose() {
  local directory="$1"
  mkdir -p "$directory"
  (cd "$directory" && "$source_copy/stellar_camera_director_v059" \
    --landmarks "$out/v059_landmarks.tsv" \
    --shots "$out/v059_shots.tsv" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1000) \
    >"$directory/director.log" 2>&1
  grep -q 'STELLAR_CAMERA_DIRECTOR_V059_OK rows=7 shots=3 dry_run=false' \
    "$directory/director.log"
}
run_v059_loose "$out/v059_run1"
run_v059_loose "$out/v059_run2"
for product in camera.tsv diagnostics.tsv manifest.txt preview.svg; do
  cmp "$out/v059_run1/$product" "$out/v059_run2/$product"
done
grep -q '^# director_schema=stellar_cinematic_director_v059$' \
  "$out/v059_run1/camera.tsv"
grep -q '^# schema=stellar_camera_diagnostics_v059$' \
  "$out/v059_run1/diagnostics.tsv"
grep -q 'transition_from transition_to' "$out/v059_run1/diagnostics.tsv"
grep -q 'roll_deg_per_frame' "$out/v059_run1/diagnostics.tsv"
grep -q 'zoom_percent_per_frame' "$out/v059_run1/diagnostics.tsv"
grep -q '^schema=stellar_cinematic_direction_manifest_v059$' \
  "$out/v059_run1/manifest.txt"
grep -q '^shot.1.transition_in=true$' "$out/v059_run1/manifest.txt"
grep -q '^shot.1.transition_end_snapshot=104$' \
  "$out/v059_run1/manifest.txt"
grep -q '^motion_budget.passed=true$' "$out/v059_run1/manifest.txt"
grep -q '^path_written=true$' "$out/v059_run1/manifest.txt"
"$source_copy/build/test_stellar_camera_path_v055" \
  "$out/v059_run1/camera.tsv" >"$out/v059_native_path_test.log" 2>&1
grep -q STELLAR_CAMERA_PATH_V055_OK "$out/v059_native_path_test.log"

mkdir -p "$out/v059_dry_rejected"
(cd "$out/v059_dry_rejected" && \
  "$source_copy/stellar_camera_director_v059" \
    --landmarks "$out/v059_landmarks.tsv" \
    --shots "$out/v059_shots.tsv" \
    --dry-run \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1 \
    --max-roll-deg-per-frame 1e-12 \
    --max-zoom-percent-per-frame 1e-12) \
    >"$out/v059_dry_rejected/director.log" 2>&1
grep -q '^motion_budget.passed=false$' \
  "$out/v059_dry_rejected/manifest.txt"
grep -q '^path_written=false$' "$out/v059_dry_rejected/manifest.txt"
test -s "$out/v059_dry_rejected/diagnostics.tsv"
test -s "$out/v059_dry_rejected/preview.svg"

mkdir -p "$out/v059_production_rejected"
set +e
(cd "$out/v059_production_rejected" && \
  "$source_copy/stellar_camera_director_v059" \
    --landmarks "$out/v059_landmarks.tsv" \
    --shots "$out/v059_shots.tsv" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1 \
    --max-roll-deg-per-frame 1e-12 \
    --max-zoom-percent-per-frame 1e-12) \
    >"$out/v059_production_rejected/director.log" 2>&1
v059_rejected_rc=$?
set -e
test "$v059_rejected_rc" -eq 2
grep -q '^STELLAR_CAMERA_DIRECTOR_V059_MOTION_REJECTED ' \
  "$out/v059_production_rejected/director.log"
test ! -e "$out/v059_production_rejected/camera.tsv"
test -s "$out/v059_production_rejected/diagnostics.tsv"
test -s "$out/v059_production_rejected/manifest.txt"
test -s "$out/v059_production_rejected/preview.svg"
grep -q '^motion_budget.passed=false$' \
  "$out/v059_production_rejected/manifest.txt"
grep -q '^path_written=false$' \
  "$out/v059_production_rejected/manifest.txt"

sha256sum "$out/v059_run1/camera.tsv" \
  "$out/v059_run1/diagnostics.tsv" \
  "$out/v059_run1/manifest.txt" \
  "$out/v059_run1/preview.svg" >"$out/v059_before_clobber.sha256"
set +e
run_v059_loose "$out/v059_run1" >"$out/v059_no_clobber_outer.log" 2>&1
v059_clobber_rc=$?
set -e
test "$v059_clobber_rc" -ne 0
grep -q 'Refusing to overwrite' "$out/v059_run1/director.log"
sha256sum "$out/v059_run1/camera.tsv" \
  "$out/v059_run1/diagnostics.tsv" \
  "$out/v059_run1/manifest.txt" \
  "$out/v059_run1/preview.svg" >"$out/v059_after_clobber.sha256"
cmp "$out/v059_before_clobber.sha256" "$out/v059_after_clobber.sha256"

mkdir -p "$out/v059_invalid_numeric"
if (cd "$out/v059_invalid_numeric" && \
    "$source_copy/stellar_camera_director_v059" \
      --landmarks "$out/v059_landmarks.tsv" \
      --shots "$out/v059_shots.tsv" \
      --dry-run \
      --diagnostics diagnostics.tsv \
      --manifest manifest.txt \
      --preview preview.svg \
      --max-roll-deg-per-frame invalid) \
      >"$out/v059_invalid_numeric/director.log" 2>&1; then
  echo "v059 director accepted invalid motion budget" >&2
  exit 16
fi
grep -q 'Invalid --max-roll-deg-per-frame' \
  "$out/v059_invalid_numeric/director.log"
test ! -e "$out/v059_invalid_numeric/diagnostics.tsv"
test ! -e "$out/v059_invalid_numeric/manifest.txt"
test ! -e "$out/v059_invalid_numeric/preview.svg"

cat >"$out/native_landmarks.tsv" <<'EOF_NATIVE_LANDMARKS'
# snapshot time cx cy cz ax ay az material_radius disk_radius polar_extent
0 0 0.5 0.5 0.5 0 1 0 0.4 0.4 0.5
EOF_NATIVE_LANDMARKS
cat >"$out/native_shots.tsv" <<'EOF_NATIVE_SHOTS'
# schema=stellar_cinematic_shots_v056
native_pose 0 0 disk_edge grid 0 0 1 - - 1 0 linear
EOF_NATIVE_SHOTS
"$source_copy/build/stellar_camera_director_v056" \
  --landmarks "$out/native_landmarks.tsv" \
  --shots "$out/native_shots.tsv" \
  --path "$out/native_camera_path.tsv" \
  --diagnostics "$out/native_camera_diagnostics.tsv" \
  --manifest "$out/native_camera_manifest.txt" \
  --preview "$out/native_camera_preview.svg" \
  --minimum-half-extent-cm 0.01 \
  >"$out/native_camera_director.log" 2>&1
grep -q 'STELLAR_CAMERA_DIRECTOR_V056_OK rows=1 shots=1 dry_run=false' \
  "$out/native_camera_director.log"
cat >"$out/native_shots_v059.tsv" <<'EOF_NATIVE_SHOTS_V059'
# schema=stellar_cinematic_shots_v059
native_pose 0 0 disk_edge grid 0 0 1 - - 1 - linear
EOF_NATIVE_SHOTS_V059
"$source_copy/stellar_camera_director_v059" \
  --landmarks "$out/native_landmarks.tsv" \
  --shots "$out/native_shots_v059.tsv" \
  --path "$out/native_camera_path_v059.tsv" \
  --diagnostics "$out/native_camera_diagnostics_v059.tsv" \
  --manifest "$out/native_camera_manifest_v059.txt" \
  --preview "$out/native_camera_preview_v059.svg" \
  --minimum-half-extent-cm 0.01 \
  >"$out/native_camera_director_v059.log" 2>&1
grep -q 'STELLAR_CAMERA_DIRECTOR_V059_OK rows=1 shots=1 dry_run=false' \
  "$out/native_camera_director_v059.log"
cmp "$out/native_camera_path.tsv" "$out/native_camera_path_v059.tsv" || {
  grep -v '^#' "$out/native_camera_path.tsv" >"$out/native_camera_v056.data"
  grep -v '^#' "$out/native_camera_path_v059.tsv" >"$out/native_camera_v059.data"
  cmp "$out/native_camera_v056.data" "$out/native_camera_v059.data"
}

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

make_palette_fixture_config() {
  local output_name="$1"
  local profile="$2"
  local config_path="$3"
  awk -v output="$output_name" -v profile="$profile" '
    $1 == "imageFile" {print "imageFile = " output; next}
    $1 == "convertUthermToKelvin" {
      print "convertUthermToKelvin = false"; next
    }
    $1 == "drawBBox" {print "drawBBox = false"; next}
    $1 == "drawTetra" {print "drawTetra = false"; next}
    $1 == "drawVoronoi" {print "drawVoronoi = false"; next}
    {print}
    END {
      print "takeLogDens = true"
      print "stellarTransferEnabled = true"
      print "stellarTransferMode = composite"
      if(profile != "-") print "stellarPaletteProfile = " profile
      print "stellarReconstruction = voronoi"
      print "stellarCenter = 0.5 0.5 0.5"
      print "stellarAxis = 0 0 1"
      print "stellarBulkVelocity = 0 0 0"
      print "stellarMaterialRadius = 0.45"
      print "stellarDiskRadius = 0.35"
      print "stellarDiskHalfThickness = 0.12"
      print "stellarPolarInner = 0.05"
      print "stellarPolarOuter = 0.49"
      print "stellarPolarConeRatio = 1.0"
      print "stellarMergerOpacity = 2.0"
      print "stellarDiskOpacity = 1.0"
      print "stellarPolarOpacity = 0.1"
      print "stellarMergerEmission = 2.0"
      print "stellarDiskEmission = 1.0"
      print "stellarPolarEmission = 0.1"
      print "stellarExposure = 1.8"
      print "stellarBlackPoint = 0.0"
      print "stellarSaturation = 0.84"
    }
  ' "$source_copy/tests/config_2.txt" >"$config_path"
}

mkdir -p "$out/artifacts/palette"
make_palette_fixture_config palette_legacy_run1.png - \
  "$source_copy/tests/config_palette_legacy_run1.txt"
make_palette_fixture_config palette_legacy_run2.png - \
  "$source_copy/tests/config_palette_legacy_run2.txt"
make_palette_fixture_config palette_copper_run1.png copper_blue_v057 \
  "$source_copy/tests/config_palette_copper_run1.txt"
make_palette_fixture_config palette_copper_run2.png copper_blue_v057 \
  "$source_copy/tests/config_palette_copper_run2.txt"
make_palette_fixture_config palette_accent_run1.png copper_blue_accent_v058 \
  "$source_copy/tests/config_palette_accent_run1.txt"
make_palette_fixture_config palette_accent_run2.png copper_blue_accent_v058 \
  "$source_copy/tests/config_palette_accent_run2.txt"
make_palette_fixture_config palette_invalid.png invalid_profile \
  "$source_copy/tests/config_palette_invalid.txt"

for name in palette_legacy_run1 palette_legacy_run2 \
            palette_copper_run1 palette_copper_run2 \
            palette_accent_run1 palette_accent_run2; do
  test ! -e "$source_copy/${name}.png"
  (cd "$source_copy" && ./ArepoRT "tests/config_${name}.txt") \
    >"$out/${name}.log" 2>&1
  test -s "$source_copy/${name}.png"
done
grep -Fq 'STELLAR_PALETTE_V057 profile=legacy_v052 id=0' \
  "$out/palette_legacy_run1.log"
grep -Fq 'polar_low=0.015,0.025,0.12 polar_high=0.32,0.48,1' \
  "$out/palette_legacy_run1.log"
grep -Fq 'composite_weights=0.42,1,0.32 polar_accent=0 neutral_overlap=0' \
  "$out/palette_legacy_run1.log"
grep -Fq 'STELLAR_PALETTE_V057 profile=copper_blue_v057 id=1' \
  "$out/palette_copper_run1.log"
grep -Fq 'polar_low=0.012,0.035,0.16 polar_high=0.16,0.34,1' \
  "$out/palette_copper_run1.log"
grep -Fq 'composite_weights=0.36,1,0.42 polar_accent=0 neutral_overlap=1' \
  "$out/palette_copper_run1.log"
grep -Fq 'STELLAR_PALETTE_V058 profile=copper_blue_accent_v058 id=2' \
  "$out/palette_accent_run1.log"
grep -Fq 'polar_low=0.08,0.035,0.008 polar_high=0.52,0.24,0.07' \
  "$out/palette_accent_run1.log"
grep -Fq 'polar_accent_low=0.012,0.028,0.14 polar_accent_high=0.08,0.18,0.75' \
  "$out/palette_accent_run1.log"
grep -Fq 'polar_accent_window=6.3,7.1,1.5e+08,3.5e+08,0.7,0.92' \
  "$out/palette_accent_run1.log"
grep -Fq 'composite_weights=0.4,1,0.28 polar_accent=1 neutral_overlap=1' \
  "$out/palette_accent_run1.log"
cmp "$source_copy/palette_legacy_run1.png" \
  "$source_copy/palette_legacy_run2.png"
cmp "$source_copy/palette_copper_run1.png" \
  "$source_copy/palette_copper_run2.png"
test "$(sha256sum "$source_copy/palette_copper_run1.png" | awk '{print $1}')" = \
  5d28175015902f57a6711f4eae8785292dd4e50134d86743c4c5d4b2a7f1eaa8
cmp "$source_copy/palette_accent_run1.png" \
  "$source_copy/palette_accent_run2.png"
set +e
(cd "$source_copy" && ./ArepoRT tests/config_palette_invalid.txt) \
  >"$out/palette_invalid.log" 2>&1
palette_invalid_rc=$?
set -e
printf 'exit_code=%s\n' "$palette_invalid_rc" \
  >"$out/palette_invalid_exit_status.txt"
grep -q 'Config: unknown stellarPaletteProfile' "$out/palette_invalid.log"
test ! -e "$source_copy/palette_invalid.png"
for name in palette_legacy_run1 palette_legacy_run2 \
            palette_copper_run1 palette_copper_run2 \
            palette_accent_run1 palette_accent_run2; do
  cp "$source_copy/${name}.png" "$out/artifacts/palette/${name}.png"
done
sha256sum "$out/artifacts/palette/"*.png \
  >"$out/palette_fixture_images.sha256"
sha256sum "$source_copy/tests/config_palette_"*.txt \
  >"$out/palette_fixture_configs.sha256"

test "$(sha256sum "$out/artifacts/run1/frame2.png" | awk '{print $1}')" = \
  29d0d5fb817680627ecde6932468d62bbe6431819f859cb745de7fd11a599555
test "$(sha256sum "$out/artifacts/run2/frame2.png" | awk '{print $1}')" = \
  29d0d5fb817680627ecde6932468d62bbe6431819f859cb745de7fd11a599555

if grep -RIl 'copper_blue_v057\|stellarPaletteProfile' \
    "$source_copy/src" --include='*.cu' --include='stellar_gpu_scene*' \
    >"$out/gpu_nonlegacy_palette_refs.txt"; then
  test ! -s "$out/gpu_nonlegacy_palette_refs.txt"
fi

read -r native_snapshot native_time camera_x camera_y camera_z \
  look_x look_y look_z up_x up_y up_z half_extent \
  center_x center_y center_z axis_x axis_y axis_z \
  material_extent disk_extent outflow_extent \
  < <(awk '!/^#/ {print; exit}' "$out/native_camera_path.tsv")
test "$native_snapshot" = 0
awk -v output="phase7_explicit_camera.png" \
    -v position="$camera_x $camera_y $camera_z" \
    -v look="$look_x $look_y $look_z" \
    -v up="$up_x $up_y $up_z" \
    -v scale="$half_extent" '
  $1 == "imageFile" {print "imageFile = " output; next}
  $1 == "cameraPosition" {print "cameraPosition = " position; next}
  $1 == "cameraLookAt" {print "cameraLookAt = " look; next}
  $1 == "cameraUp" {print "cameraUp = " up; next}
  $1 == "swScale" {print "swScale = " scale; next}
  {print}
' "$source_copy/tests/config_2.txt" \
  >"$source_copy/tests/config_phase7_explicit_camera.txt"
awk -v output="phase7_decoy_camera.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  $1 == "cameraPosition" {print "cameraPosition = 1.5 0.5 0.5"; next}
  $1 == "cameraLookAt" {print "cameraLookAt = 0.5 0.5 0.5"; next}
  $1 == "cameraUp" {print "cameraUp = 0 0 1"; next}
  $1 == "swScale" {print "swScale = 0.8"; next}
  {print}
' "$source_copy/tests/config_phase7_explicit_camera.txt" \
  >"$source_copy/tests/config_phase7_decoy_camera.txt"
awk -v output="phase7_native_camera.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  {print}
' "$source_copy/tests/config_phase7_decoy_camera.txt" \
  >"$source_copy/tests/config_phase7_native_camera.txt"
{
  echo "stellarCameraPath = $out/native_camera_path.tsv"
  echo "stellarCameraDirectionManifest = $out/native_camera_manifest.txt"
} >>"$source_copy/tests/config_phase7_native_camera.txt"
awk -v output="phase10_native_camera.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  {print}
' "$source_copy/tests/config_phase7_decoy_camera.txt" \
  >"$source_copy/tests/config_phase10_native_camera.txt"
{
  echo "stellarCameraPath = $out/native_camera_path_v059.tsv"
  echo "stellarCameraDirectionManifest = $out/native_camera_manifest_v059.txt"
} >>"$source_copy/tests/config_phase10_native_camera.txt"
rm -f "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase7_decoy_camera.png" \
  "$source_copy/phase7_native_camera.png" \
  "$source_copy/phase10_native_camera.png"
(cd "$source_copy" && ./ArepoRT tests/config_phase7_explicit_camera.txt) \
  >"$out/native_explicit_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase7_decoy_camera.txt) \
  >"$out/native_decoy_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase7_native_camera.txt) \
  >"$out/native_manifest_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase10_native_camera.txt) \
  >"$out/native_manifest_v059_render.log" 2>&1
test -s "$source_copy/phase7_explicit_camera.png"
test -s "$source_copy/phase7_decoy_camera.png"
test -s "$source_copy/phase7_native_camera.png"
test -s "$source_copy/phase10_native_camera.png"
if cmp -s "$source_copy/phase7_explicit_camera.png" \
    "$source_copy/phase7_decoy_camera.png"; then
  echo "decoy camera did not change the reference image" >&2
  exit 15
fi
cmp "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase7_native_camera.png"
cmp "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase10_native_camera.png"
grep -q '^STELLAR_CAMERA_MANIFEST_V056 .*schema=stellar_cinematic_direction_manifest_v056$' \
  "$out/native_manifest_render.log"
grep -q '^STELLAR_CAMERA_MANIFEST_V059 .*schema=stellar_cinematic_direction_manifest_v059$' \
  "$out/native_manifest_v059_render.log"
grep -q '^STELLAR_CAMERA_PATH_V055 snapshot=0 position=' \
  "$out/native_manifest_v059_render.log"
grep -q ' look_at=' "$out/native_manifest_v059_render.log"
grep -q ' up=' "$out/native_manifest_v059_render.log"
grep -q ' half_extent=' "$out/native_manifest_v059_render.log"
grep -q '^STELLAR_CAMERA_PATH_V055 snapshot=0 position=' \
  "$out/native_manifest_render.log"
grep -q ' look_at=' "$out/native_manifest_render.log"
grep -q ' up=' "$out/native_manifest_render.log"
grep -q ' half_extent=' "$out/native_manifest_render.log"
cp "$source_copy/phase7_explicit_camera.png" \
  "$out/artifacts/phase7_explicit_camera.png"
cp "$source_copy/phase7_decoy_camera.png" \
  "$out/artifacts/phase7_decoy_camera.png"
cp "$source_copy/phase7_native_camera.png" \
  "$out/artifacts/phase7_native_camera.png"
cp "$source_copy/phase10_native_camera.png" \
  "$out/artifacts/phase10_native_camera.png"
sha256sum "$out/artifacts/phase7_explicit_camera.png" \
  "$out/artifacts/phase7_decoy_camera.png" \
  "$out/artifacts/phase7_native_camera.png" \
  "$out/artifacts/phase10_native_camera.png" \
  >"$out/native_manifest_parity.sha256"

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
  "$source_copy/stellar_camera_director_v056" \
  "$source_copy/stellar_camera_director_v059" \
  "$source_copy/test_stellar_cinematic_director_v059" \
  "$source_copy/build/test_stellar_render_model_v051a" \
  "$source_copy/build/test_stellar_render_model_v052a" \
  "$source_copy/build/test_stellar_gpu_scene_format_v052" \
  "$source_copy/build/test_stellar_scene_physics_v052" \
  "$source_copy/build/test_stellar_reconstruction_v053" \
  "$source_copy/build/test_stellar_camera_v054" \
  "$source_copy/build/stellar_camera_plan_v054" \
  "$source_copy/build/test_stellar_camera_path_v055" \
  "$source_copy/build/test_stellar_cinematic_director_v056" \
  "$source_copy/build/stellar_camera_director_v056" \
  "$source_copy/test_stellar_palette_v057" \
  "$source_copy/test_stellar_palette_v058" \
  "$source_copy/build/test_stellar_field_registry" \
  "$source_copy/build/stellar_gpu_renderer_v051a" \
  "$source_copy/build/stellar_gpu_renderer_v052a" \
  "$source_copy/build/stellar_gpu_renderer_v053a" >"$out/binaries.sha256"
sha256sum "$out/cinematic_path_run1.tsv" \
  "$out/cinematic_diagnostics_run1.tsv" \
  "$out/cinematic_manifest_run1.txt" \
  "$out/cinematic_preview_run1.svg" \
  "$out/native_camera_path.tsv" \
  "$out/native_camera_manifest.txt" \
  "$out/v059_run1/camera.tsv" \
  "$out/v059_run1/diagnostics.tsv" \
  "$out/v059_run1/manifest.txt" \
  "$out/v059_run1/preview.svg" \
  "$out/native_camera_path_v059.tsv" \
  "$out/native_camera_manifest_v059.txt" \
  >"$out/cinematic_products.sha256"
sha256sum "$out/artifacts/palette/"*.png \
  "$out/palette_fixture_configs.sha256" \
  "$out/palette_fixture_images.sha256" \
  "$out/palette_v057_test.log" \
  "$out/palette_v058_test.log" \
  "$out/palette_legacy_run1.log" \
  "$out/palette_copper_run1.log" \
  "$out/palette_accent_run1.log" \
  "$out/palette_invalid.log" \
  "$out/palette_invalid_exit_status.txt" \
  "$out/gpu_nonlegacy_palette_refs.txt" >"$out/palette_products.sha256"
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
  echo "native_camera_path_v055_tests=1"
  echo "native_camera_snapshot_lookup=true"
  echo "native_camera_path_strict_geometry=true"
  echo "native_camera_path_keyframe_conflict_guard=true"
  echo "stellar_cinematic_director_v056_tests=1"
  echo "camera_named_options=true"
  echo "camera_multishot_contiguous=true"
  echo "camera_explicit_elevation_and_framing=true"
  echo "camera_orbit_amount_or_period=true"
  echo "camera_lobe_independent_of_orbit=true"
  echo "camera_transitions=linear,smoothstep,smootherstep"
  echo "camera_dry_run_preview=true"
  echo "camera_motion_coverage_diagnostics=true"
  echo "camera_direction_manifest_bound_native=true"
  echo "camera_native_log_fields=position,look_at,up,scale"
  echo "camera_native_manifest_pixel_parity=true"
  echo "camera_outputs_no_clobber=true"
  echo "camera_cli_numeric_and_option_strict=true"
  echo "stellar_cinematic_director_v059_tests=1"
  echo "camera_transition_semantics=incoming_explicit_snapshot_window"
  echo "camera_transition_does_not_consume_outgoing_shot=true"
  echo "camera_transition_window_boundaries_strict=true"
  echo "camera_motion_budget_defaults=roll_0.25_deg_per_frame,zoom_1_percent_per_frame"
  echo "camera_motion_budget_worst_snapshots_reported=true"
  echo "camera_rejected_production_path_absent=true"
  echo "camera_rejected_dry_run_review_products_present=true"
  echo "camera_v059_path_native_v055_compatible=true"
  echo "camera_v059_manifest_bound_native=true"
  echo "camera_v056_manifest_behavior_retained=true"
  echo "stellar_palette_v057_tests=1"
  echo "palette_default=legacy_v052"
  echo "palette_legacy_constants_exact=true"
  echo "palette_copper_blue_opt_in=true"
  echo "palette_physical_layer_selection_unchanged=true"
  echo "palette_native_provenance_logged=true"
  echo "palette_invalid_profile_rejected=true"
  echo "palette_invalid_profile_exit_code=$palette_invalid_rc"
  echo "palette_invalid_profile_exit_semantics=historical_terminate_exit_zero"
  echo "palette_tiny_fixture_deterministic=true"
  echo "stellar_palette_v058_tests=1"
  echo "palette_copper_blue_accent_opt_in=true"
  echo "palette_bulk_wind_warm=true"
  echo "palette_hot_fast_coherent_wind_cobalt=true"
  echo "palette_v057_fixture_exact=true"
  echo "palette_v057_v058_physical_extinction_equal=true"
  echo "palette_gpu_contract=legacy_only"
  echo "candidate_branch=$expected_branch"
  echo "candidate_base=$expected_base"
  echo "candidate_worktree_surface_exact=true"
  echo "candidate_hashes_verified=true"
  echo "native_scene_exporter_build=true"
  echo "stellar_field_registry_tests=1"
  echo "gpu_renderer_v051_build=true"
  echo "gpu_renderer_v052_build=true"
  echo "gpu_renderer_v053_build=true"
  echo "gpu_renderer_executed=false"
  echo "voronoi_reference_modified=false"
} >"$out/summary.txt"
touch "$out/PHASE10_CAMERA_VALIDATE_OK"
cat "$out/summary.txt"
echo "PHASE10_CAMERA_VALIDATE_OK output=$out host=$host job=$SLURM_JOB_ID"
