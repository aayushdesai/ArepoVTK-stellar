#!/bin/bash
#SBATCH --job-name=arepovtk_stellar_p19
#SBATCH --account=caiazgrp
#SBATCH --partition=defaultp
#SBATCH --constraint=eta
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --threads-per-core=1
#SBATCH --mem=96G
#SBATCH --time=01:00:00
#SBATCH --output=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase19_validate_%j.out
#SBATCH --error=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase19_validate_%j.err

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
expected_branch=stellar/phase19-signed-framing-20260827T061500Z
expected_base=db97fbba56b3b23223108996b3421a9e4abec307
out="$repo/validation/phase19_${SLURM_JOB_ID}"
source_copy="$out/source"
[[ ! -e "$out" ]] || exit 5
[[ "$(git -C "$repo" rev-parse "$reference_commit")" == "$reference_commit" ]] || exit 6
[[ "$(git -C "$dependency" rev-parse HEAD)" == "$arepo_commit" ]] || exit 7
[[ -z "$(git -C "$dependency" status --porcelain)" ]] || exit 8
mkdir -p "$out" "$source_copy" "$out/artifacts/run1" "$out/artifacts/run2"

{
  echo "phase=19"
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
  ' M src/stellar_feature_diagnostics_v064.cpp' \
  ' M src/stellar_feature_diagnostics_v064.h' \
  ' M src/stellar_feature_probe_v064.cpp' \
  ' M tests/test_stellar_feature_diagnostics_v064.cpp' \
  '?? docs/stellar_feature_framing_v067.md' \
  '?? slurm/phase19_validate.sbatch' \
  '?? src/stellar_feature_framing_v067.cpp' \
  '?? src/stellar_feature_framing_v067.h' \
  '?? tests/test_stellar_feature_framing_v067.cpp' \
  >"$out/expected_repo_status.txt"
cmp "$out/expected_repo_status.txt" "$out/repo_status.txt"
test ! -s "$out/diff_check.txt"
printf '%s\n' \
  'docs/stellar_feature_framing_v067.md' \
  'slurm/phase19_validate.sbatch' \
  'src/stellar_feature_framing_v067.cpp' \
  'src/stellar_feature_framing_v067.h' \
  'tests/test_stellar_feature_framing_v067.cpp' \
  >"$out/expected_untracked_files.txt"
cmp "$out/expected_untracked_files.txt" "$out/untracked_files.txt"
test "$(sha256sum "$out/candidate.diff" | awk '{print $1}')" = \
  e717c2aa62b9deb150b21dad900c677f796b6f55a55e04cc70d4d0ccd4fb9fb1
sha256sum "$repo/slurm/phase19_validate.sbatch" >"$out/validator.sha256"
(
  cd "$repo"
  printf '%s  %s\n' \
    b6ec25ac082862f33b11ea5ad3a2400cf7bf66c6fc8fc0bac6e21064d64daef9 Makefile \
    21fc499265a2c1d161d4d3f7c577f694a5abba4e9fa15ae68ee19a1de6734a7f src/stellar_feature_diagnostics_v064.cpp \
    d24fbc34b90b08efaa3334070368e054fa8d3e1399dbc5c8cb6279db85cffd40 src/stellar_feature_diagnostics_v064.h \
    d5486bd774261e87fe7d57b902be238e4307cdfb566a5e6b8156726e1456a442 src/stellar_feature_framing_v067.cpp \
    cb35190d94e0c4b064991bdb2860c737c7c49702f74960d36d81ffb1a89ee325 src/stellar_feature_framing_v067.h \
    0cd3b764413f0eae5fd7a4f5dad333672e7a572c40defae40ccfccaa3a13a3a2 src/stellar_feature_probe_v064.cpp \
    489e0b169d823846eb6de78db8c9bbc767d8c7aa6f598f90ff902c4991262852 tests/test_stellar_feature_diagnostics_v064.cpp \
    e3d388620de7c7e5706ff8bd69158f2bc89806b422698a56160ecfe80b9275ac tests/test_stellar_feature_framing_v067.cpp \
    e0302dae3fcb9d56a0c65b4a79c825732039c64c5e6602761a944c636c276f5e docs/stellar_feature_framing_v067.md \
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

make -C "$source_copy" stellar_feature_probe_v064 \
  test_stellar_feature_diagnostics_v064 \
  test_stellar_feature_framing_v067 \
  test_stellar_feature_landmarks_v066 \
  test_stellar_feature_profile_v065 \
  >"$out/feature_diagnostics_make.log" 2>&1
test -x "$source_copy/stellar_feature_probe_v064"
test -x "$source_copy/test_stellar_feature_diagnostics_v064"
test -x "$source_copy/test_stellar_feature_framing_v067"
test -x "$source_copy/test_stellar_feature_landmarks_v066"
test -x "$source_copy/test_stellar_feature_profile_v065"
mkdir -p "$out/landmark_unit"
mkdir -p "$out/framing_unit"
"$source_copy/test_stellar_feature_framing_v067" "$out/framing_unit" \
  >"$out/feature_framing_v067_test.log" 2>&1
grep -q '^STELLAR_FEATURE_FRAMING_V067_OK bipolar_center_y=0.35 bipolar_scale=1.4375 no_camera_mutation=1$' \
  "$out/feature_framing_v067_test.log"
grep -q '^# schema=stellar_feature_framing_plan_v067$' \
  "$out/framing_unit/framing_plan_v067.tsv"
"$source_copy/test_stellar_feature_landmarks_v066" "$out/landmark_unit" \
  >"$out/feature_landmarks_v066_test.log" 2>&1
grep -q '^STELLAR_FEATURE_LANDMARKS_V066_OK q99=0.4 raw_outlier=4$' \
  "$out/feature_landmarks_v066_test.log"
grep -q '^# schema=stellar_feature_landmarks_v066$' \
  "$out/landmark_unit/landmark_fixture_v066.tsv"
"$source_copy/test_stellar_feature_profile_v065" \
  >"$out/feature_profile_v065_test.log" 2>&1
grep -q '^STELLAR_FEATURE_PROFILE_V065_OK ' \
  "$out/feature_profile_v065_test.log"
mkdir -p "$out/feature_fixture_run1" "$out/feature_fixture_run2"
"$source_copy/test_stellar_feature_diagnostics_v064" \
  "$out/feature_fixture_run1" >"$out/feature_fixture_run1.log" 2>&1
"$source_copy/test_stellar_feature_diagnostics_v064" \
  "$out/feature_fixture_run2" >"$out/feature_fixture_run2.log" 2>&1
grep -q '^STELLAR_FEATURE_DIAGNOSTICS_V064_OK disk_cells=2 ' \
  "$out/feature_fixture_run1.log"
grep -q 'disk_core_fraction=' "$out/feature_fixture_run1.log"
grep -q 'disk_merger_overlap=' "$out/feature_fixture_run1.log"
grep -q 'polar_positive=1 polar_negative=1$' \
  "$out/feature_fixture_run1.log"
cmp "$out/feature_fixture_run1/feature_scene_v064.bin" \
  "$out/feature_fixture_run2/feature_scene_v064.bin"
cmp "$out/feature_fixture_run1/feature_scene_quantized_v064.bin" \
  "$out/feature_fixture_run2/feature_scene_quantized_v064.bin"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run1/feature_report_v064.tsv" \
  >"$out/feature_fixture_run1.normalized.tsv"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run2/feature_report_v064.tsv" \
  >"$out/feature_fixture_run2.normalized.tsv"
cmp "$out/feature_fixture_run1.normalized.tsv" \
  "$out/feature_fixture_run2.normalized.tsv"
accepted_phase17_fixture="$repo/validation/phase17_exact_63749204/feature_fixture_run1.normalized.tsv"
test -s "$accepted_phase17_fixture"
test "$(sha256sum "$accepted_phase17_fixture" | awk '{print $1}')" = \
  84aeaaa2ddff17a034c1296dc9939e4caa2287e9ebb42fa34a686c46db3148c6
sed -E 's|^# scene=.*$|# scene=fixture/feature_scene_v064.bin|' \
  "$accepted_phase17_fixture" >"$out/accepted_phase17_feature_v064.canonical.tsv"
sed -E 's|^# scene=.*$|# scene=fixture/feature_scene_v064.bin|' \
  "$out/feature_fixture_run1.normalized.tsv" \
  >"$out/current_feature_v064.canonical.tsv"
cmp "$out/accepted_phase17_feature_v064.canonical.tsv" \
  "$out/current_feature_v064.canonical.tsv"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run1/feature_report_structures_v065.tsv" \
  >"$out/feature_structures_run1.normalized.tsv"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run2/feature_report_structures_v065.tsv" \
  >"$out/feature_structures_run2.normalized.tsv"
cmp "$out/feature_structures_run1.normalized.tsv" \
  "$out/feature_structures_run2.normalized.tsv"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run1/feature_landmarks_v066.tsv" \
  >"$out/feature_landmarks_run1.normalized.tsv"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run2/feature_landmarks_v066.tsv" \
  >"$out/feature_landmarks_run2.normalized.tsv"
cmp "$out/feature_landmarks_run1.normalized.tsv" \
  "$out/feature_landmarks_run2.normalized.tsv"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run1/feature_landmarks_structures_v066.tsv" \
  >"$out/feature_landmarks_structures_run1.normalized.tsv"
sed 's#feature_fixture_run[12]#feature_fixture_run#g' \
  "$out/feature_fixture_run2/feature_landmarks_structures_v066.tsv" \
  >"$out/feature_landmarks_structures_run2.normalized.tsv"
cmp "$out/feature_landmarks_structures_run1.normalized.tsv" \
  "$out/feature_landmarks_structures_run2.normalized.tsv"
accepted_phase18="$repo/validation/phase18_exact_63749264"
test -f "$accepted_phase18/PHASE18_FEATURE_LANDMARK_VALIDATE_OK"
for product in \
  feature_fixture_run1.normalized.tsv \
  feature_structures_run1.normalized.tsv \
  feature_landmarks_run1.normalized.tsv \
  feature_landmarks_structures_run1.normalized.tsv; do
  test -s "$accepted_phase18/$product"
  sed -E 's|^# scene=.*$|# scene=fixture/feature_scene_v064.bin|' \
    "$accepted_phase18/$product" >"$out/accepted_phase18_${product}"
  sed -E 's|^# scene=.*$|# scene=fixture/feature_scene_v064.bin|' \
    "$out/$product" >"$out/current_phase19_${product}"
  cmp "$out/accepted_phase18_${product}" \
    "$out/current_phase19_${product}"
done
grep -q '^# feature_profile=legacy_v064$' \
  "$out/feature_fixture_run1/feature_report_v064.tsv"
grep -q '^# feature_profile=stellar_structures_v065$' \
  "$out/feature_fixture_run1/feature_report_structures_v065.tsv"
awk -F '\t' '
  $1 == "disk" && $2 > 0.009 && $2 < 0.011 {
    if ($3 != 2 || !($5 > 0) || !($6 > 0) || !($7 > 0)) exit 1
    disk = 1
  }
  $1 == "polar_positive" && $2 > 0.009 && $2 < 0.011 {
    if ($3 != 1 || $19 != 1) exit 2
    positive = 1
  }
  $1 == "polar_negative" && $2 > 0.009 && $2 < 0.011 {
    if ($3 != 1 || $19 != 1) exit 3
    negative = 1
  }
  END {if (!disk || !positive || !negative) exit 4}
' "$out/feature_fixture_run1/feature_report_v064.tsv"

feature_probe_common=(
  --scene "$out/feature_fixture_run1/feature_scene_v064.bin"
  --center 0 0 0 --axis 0 0 1 --bulk 0 0 0
  --box-size 1e12 --material-radius 2e10
  --disk-radius 4e10 --disk-half-thickness 8e9
  --polar-inner 5e9 --polar-outer 8e10 --polar-cone-ratio 0.5
  --minimum-weights 0.01
)
"$source_copy/stellar_feature_probe_v064" "${feature_probe_common[@]}" \
  --output "$out/feature_cli_run1.tsv" >"$out/feature_cli_run1.log" 2>&1
"$source_copy/stellar_feature_probe_v064" "${feature_probe_common[@]}" \
  --output "$out/feature_cli_run2.tsv" >"$out/feature_cli_run2.log" 2>&1
grep -q '^STELLAR_FEATURE_PROBE_V064_OK .* feature_profile=legacy_v064 cells=4 rays=16 rows=5 orthographic=1$' \
  "$out/feature_cli_run1.log"
cmp "$out/feature_cli_run1.tsv" "$out/feature_cli_run2.tsv"
feature_structures_probe_common=(
  "${feature_probe_common[@]}"
  --feature-profile stellar_structures_v065
)
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_structures_cli_run1.tsv" \
  >"$out/feature_structures_cli_run1.log" 2>&1
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_structures_cli_run2.tsv" \
  >"$out/feature_structures_cli_run2.log" 2>&1
grep -q '^STELLAR_FEATURE_PROBE_V064_OK .* feature_profile=stellar_structures_v065 cells=4 rays=16 rows=5 orthographic=1$' \
  "$out/feature_structures_cli_run1.log"
cmp "$out/feature_structures_cli_run1.tsv" \
  "$out/feature_structures_cli_run2.tsv"
grep -q '^# feature_profile=stellar_structures_v065$' \
  "$out/feature_structures_cli_run1.tsv"
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_paired_cli_run1.tsv" \
  --landmark-output "$out/feature_landmark_cli_run1.tsv" \
  >"$out/feature_paired_cli_run1.log" 2>&1
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_paired_cli_run2.tsv" \
  --landmark-output "$out/feature_landmark_cli_run2.tsv" \
  >"$out/feature_paired_cli_run2.log" 2>&1
grep -q '^STELLAR_FEATURE_PROBE_V064_OK .* feature_profile=stellar_structures_v065 cells=4 rays=16 rows=5 orthographic=1 landmark_output=.*feature_landmark_cli_run1.tsv$' \
  "$out/feature_paired_cli_run1.log"
cmp "$out/feature_paired_cli_run1.tsv" "$out/feature_paired_cli_run2.tsv"
cmp "$out/feature_landmark_cli_run1.tsv" "$out/feature_landmark_cli_run2.tsv"
grep -q '^# schema=stellar_feature_landmarks_v066$' \
  "$out/feature_landmark_cli_run1.tsv"
grep -q '^# projection_support=all_selected_cell_centers_unclipped$' \
  "$out/feature_landmark_cli_run1.tsv"
grep -q '^# feature_profile=stellar_structures_v065$' \
  "$out/feature_landmark_cli_run1.tsv"
awk -F '\t' '
  !/^#/ && $1 != "feature" {
    rows++
    if (NF != 24 || $3 <= 0 || $4 <= 0) exit 1
    for (column = 5; column <= 23; ++column)
      if (tolower($column) ~ /nan|inf/) exit 2
    if (!($7 <= $8 && $8 <= $9)) exit 3
    if (!($10 <= $11 && $11 <= $12)) exit 4
    if (!($13 <= $14 && $14 <= $15)) exit 5
    if (!($16 <= $17 && $17 <= $18)) exit 6
    if (!($19 <= $20 && $20 <= $21)) exit 7
    if ($24 != 0 && $24 != 1) exit 8
  }
  END {if (rows != 5) exit 9}
' "$out/feature_landmark_cli_run1.tsv"
sha256sum "$out/feature_paired_cli_run1.tsv" \
  "$out/feature_landmark_cli_run1.tsv" \
  >"$out/feature_paired_before_clobber.sha256"
set +e
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_paired_cli_run1.tsv" \
  --landmark-output "$out/feature_landmark_cli_run1.tsv" \
  >"$out/feature_paired_clobber.log" 2>&1
feature_paired_clobber_rc=$?
set -e
test "$feature_paired_clobber_rc" -eq 3
grep -q 'refusing to overwrite profile, landmark, or framing output' \
  "$out/feature_paired_clobber.log"
sha256sum "$out/feature_paired_cli_run1.tsv" \
  "$out/feature_landmark_cli_run1.tsv" \
  >"$out/feature_paired_after_clobber.sha256"
cmp "$out/feature_paired_before_clobber.sha256" \
  "$out/feature_paired_after_clobber.sha256"
set +e
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_same_output_path.tsv" \
  --landmark-output "$out/feature_same_output_path.tsv" \
  >"$out/feature_same_output_path.log" 2>&1
feature_same_output_rc=$?
set -e
test "$feature_same_output_rc" -eq 3
grep -q 'refusing to overwrite profile, landmark, or framing output' \
  "$out/feature_same_output_path.log"
test ! -e "$out/feature_same_output_path.tsv"

framing_common=(
  --framing-mode bipolar_union
  --framing-features polar_positive,polar_negative
  --framing-minimum-weight 0.01
  --framing-coverage 0.98
  --framing-target-half-width 0.80
  --framing-target-half-height 0.80
)
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" "${framing_common[@]}" \
  --output "$out/feature_framing_profile_run1.tsv" \
  --landmark-output "$out/feature_framing_landmarks_run1.tsv" \
  --framing-output "$out/feature_framing_plan_run1.tsv" \
  >"$out/feature_framing_cli_run1.log" 2>&1
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" "${framing_common[@]}" \
  --output "$out/feature_framing_profile_run2.tsv" \
  --landmark-output "$out/feature_framing_landmarks_run2.tsv" \
  --framing-output "$out/feature_framing_plan_run2.tsv" \
  >"$out/feature_framing_cli_run2.log" 2>&1
grep -q '^STELLAR_FEATURE_PROBE_V064_OK .* feature_profile=stellar_structures_v065 cells=4 rays=16 rows=5 orthographic=1 landmark_output=.*feature_framing_landmarks_run1.tsv framing_output=.*feature_framing_plan_run1.tsv$' \
  "$out/feature_framing_cli_run1.log"
cmp "$out/feature_framing_profile_run1.tsv" \
  "$out/feature_framing_profile_run2.tsv"
cmp "$out/feature_framing_landmarks_run1.tsv" \
  "$out/feature_framing_landmarks_run2.tsv"
cmp "$out/feature_framing_plan_run1.tsv" \
  "$out/feature_framing_plan_run2.tsv"
grep -q '^# schema=stellar_feature_framing_plan_v067$' \
  "$out/feature_framing_plan_run1.tsv"
grep -q '^# projection_support=all_selected_cell_centers_unclipped$' \
  "$out/feature_framing_plan_run1.tsv"
grep -q '^# feature_profile=stellar_structures_v065$' \
  "$out/feature_framing_plan_run1.tsv"
grep -q '^# mode=bipolar_union$' \
  "$out/feature_framing_plan_run1.tsv"
grep -q '^# recommendation_scope=diagnostic_only_no_camera_mutation$' \
  "$out/feature_framing_plan_run1.tsv"
python - "$out/feature_framing_plan_run1.tsv" <<'PY'
import csv
import math
import sys

path = sys.argv[1]
metadata = {}
table = []
with open(path, encoding="utf-8") as stream:
    lines = stream.readlines()
for line in lines:
    if line.startswith("# ") and "=" in line:
        key, value = line[2:].rstrip("\n").split("=", 1)
        metadata[key] = value
header_index = next(i for i, line in enumerate(lines) if not line.startswith("#"))
table = list(csv.DictReader(lines[header_index:], delimiter="\t"))
if [row["feature"] for row in table] != ["polar_positive", "polar_negative"]:
    raise SystemExit("unexpected framing members")
for row in table:
    if int(row["selected_cells"]) <= 0 or float(row["selected_weight"]) <= 0:
        raise SystemExit("empty framing member")
x_low = min(float(row["x_low"]) for row in table)
x_high = max(float(row["x_high"]) for row in table)
y_low = min(float(row["y_low"]) for row in table)
y_high = max(float(row["y_high"]) for row in table)
expected = {
    "combined_x_low": x_low,
    "combined_x_high": x_high,
    "combined_y_low": y_low,
    "combined_y_high": y_high,
    "look_at_shift_screen_x_fraction": 0.5 * (x_low + x_high),
    "look_at_shift_screen_y_fraction": 0.5 * (y_low + y_high),
    "robust_half_width_fraction": 0.5 * (x_high - x_low),
    "robust_half_height_fraction": 0.5 * (y_high - y_low),
}
expected["half_extent_scale_factor"] = max(
    expected["robust_half_width_fraction"] / 0.8,
    expected["robust_half_height_fraction"] / 0.8,
)
for key, value in expected.items():
    actual = float(metadata[key])
    if not math.isfinite(actual) or not math.isclose(actual, value, rel_tol=1e-12, abs_tol=1e-12):
        raise SystemExit(f"framing metadata mismatch: {key}")
if expected["half_extent_scale_factor"] <= 0:
    raise SystemExit("non-positive framing scale")
PY

sha256sum "$out/feature_framing_profile_run1.tsv" \
  "$out/feature_framing_landmarks_run1.tsv" \
  "$out/feature_framing_plan_run1.tsv" \
  >"$out/feature_framing_before_clobber.sha256"
set +e
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" "${framing_common[@]}" \
  --output "$out/feature_framing_profile_run1.tsv" \
  --landmark-output "$out/feature_framing_landmarks_run1.tsv" \
  --framing-output "$out/feature_framing_plan_run1.tsv" \
  >"$out/feature_framing_clobber.log" 2>&1
feature_framing_clobber_rc=$?
set -e
test "$feature_framing_clobber_rc" -eq 3
grep -q 'refusing to overwrite profile, landmark, or framing output' \
  "$out/feature_framing_clobber.log"
sha256sum "$out/feature_framing_profile_run1.tsv" \
  "$out/feature_framing_landmarks_run1.tsv" \
  "$out/feature_framing_plan_run1.tsv" \
  >"$out/feature_framing_after_clobber.sha256"
cmp "$out/feature_framing_before_clobber.sha256" \
  "$out/feature_framing_after_clobber.sha256"

set +e
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" "${framing_common[@]}" \
  --output "$out/feature_framing_alias.tsv" \
  --framing-output "$out/feature_framing_alias.tsv" \
  >"$out/feature_framing_alias.log" 2>&1
feature_framing_alias_rc=$?
set -e
test "$feature_framing_alias_rc" -eq 3
grep -q 'refusing to overwrite profile, landmark, or framing output' \
  "$out/feature_framing_alias.log"
test ! -e "$out/feature_framing_alias.tsv"

set +e
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_framing_incomplete_profile.tsv" \
  --framing-output "$out/feature_framing_incomplete_plan.tsv" \
  --framing-mode single \
  >"$out/feature_framing_incomplete.log" 2>&1
feature_framing_incomplete_rc=$?
set -e
test "$feature_framing_incomplete_rc" -eq 2
grep -q 'incomplete framing request' "$out/feature_framing_incomplete.log"
test ! -e "$out/feature_framing_incomplete_profile.tsv"
test ! -e "$out/feature_framing_incomplete_plan.tsv"

set +e
"$source_copy/stellar_feature_probe_v064" \
  "${feature_structures_probe_common[@]}" \
  --output "$out/feature_framing_missing_profile.tsv" \
  --framing-output "$out/feature_framing_missing_plan.tsv" \
  --framing-mode single --framing-features not_a_feature \
  --framing-minimum-weight 0.01 --framing-coverage 0.98 \
  --framing-target-half-width 0.8 --framing-target-half-height 0.8 \
  >"$out/feature_framing_missing.log" 2>&1
feature_framing_missing_rc=$?
set -e
test "$feature_framing_missing_rc" -eq 3
grep -q 'missing framing feature row' "$out/feature_framing_missing.log"
test ! -e "$out/feature_framing_missing_profile.tsv"
test ! -e "$out/feature_framing_missing_plan.tsv"

feature_quantized_probe_common=(
  --scene "$out/feature_fixture_run1/feature_scene_quantized_v064.bin"
  --center 0 0 0 --axis 0 0 1 --bulk 0 0 0
  --box-size 1e12 --material-radius 2e10
  --disk-radius 4e10 --disk-half-thickness 8e9
  --polar-inner 5e9 --polar-outer 8e10 --polar-cone-ratio 0.5
  --minimum-weights 0.01
)
"$source_copy/stellar_feature_probe_v064" \
  "${feature_quantized_probe_common[@]}" \
  --output "$out/feature_quantized_cli_run1.tsv" \
  >"$out/feature_quantized_cli_run1.log" 2>&1
"$source_copy/stellar_feature_probe_v064" \
  "${feature_quantized_probe_common[@]}" \
  --output "$out/feature_quantized_cli_run2.tsv" \
  >"$out/feature_quantized_cli_run2.log" 2>&1
grep -q '^STELLAR_FEATURE_PROBE_V064_OK .* feature_profile=legacy_v064 cells=4 rays=16 rows=5 orthographic=1$' \
  "$out/feature_quantized_cli_run1.log"
cmp "$out/feature_quantized_cli_run1.tsv" \
  "$out/feature_quantized_cli_run2.tsv"
sha256sum "$out/feature_cli_run1.tsv" >"$out/feature_cli_before_clobber.sha256"
set +e
"$source_copy/stellar_feature_probe_v064" "${feature_probe_common[@]}" \
  --output "$out/feature_cli_run1.tsv" >"$out/feature_cli_clobber.log" 2>&1
feature_clobber_rc=$?
set -e
test "$feature_clobber_rc" -eq 3
grep -q 'refusing to overwrite output' "$out/feature_cli_clobber.log"
sha256sum "$out/feature_cli_run1.tsv" >"$out/feature_cli_after_clobber.sha256"
cmp "$out/feature_cli_before_clobber.sha256" \
  "$out/feature_cli_after_clobber.sha256"

set +e
"$source_copy/stellar_feature_probe_v064" \
  --scene "$out/feature_fixture_run1/feature_scene_v064.bin" \
  --output "$out/feature_invalid_numeric.tsv" \
  --center 0 0 0 --axis 0 0 1 --bulk 0 0 0 \
  --box-size 1e12 --material-radius 2e10 --disk-radius invalid \
  --disk-half-thickness 8e9 --polar-inner 5e9 --polar-outer 8e10 \
  --polar-cone-ratio 0.5 >"$out/feature_invalid_numeric.log" 2>&1
feature_invalid_numeric_rc=$?
set -e
test "$feature_invalid_numeric_rc" -eq 2
grep -q 'invalid disk radius' "$out/feature_invalid_numeric.log"
test ! -e "$out/feature_invalid_numeric.tsv"

set +e
"$source_copy/stellar_feature_probe_v064" \
  "${feature_probe_common[@]}" --feature-profile invalid \
  --output "$out/feature_invalid_profile.tsv" \
  >"$out/feature_invalid_profile.log" 2>&1
feature_invalid_profile_rc=$?
set -e
test "$feature_invalid_profile_rc" -eq 2
grep -q 'invalid feature profile' "$out/feature_invalid_profile.log"
test ! -e "$out/feature_invalid_profile.tsv"

head -c 240 "$out/feature_fixture_run1/feature_scene_v064.bin" \
  >"$out/feature_truncated_scene.bin"
feature_truncated_common=(
  --scene "$out/feature_truncated_scene.bin"
  --center 0 0 0 --axis 0 0 1 --bulk 0 0 0
  --box-size 1e12 --material-radius 2e10
  --disk-radius 4e10 --disk-half-thickness 8e9
  --polar-inner 5e9 --polar-outer 8e10 --polar-cone-ratio 0.5
)
set +e
"$source_copy/stellar_feature_probe_v064" "${feature_truncated_common[@]}" \
  --output "$out/feature_truncated.tsv" >"$out/feature_truncated.log" 2>&1
feature_truncated_rc=$?
set -e
test "$feature_truncated_rc" -eq 3
grep -q 'truncated stellar cell array' "$out/feature_truncated.log"
test ! -e "$out/feature_truncated.tsv"

set +e
"$source_copy/stellar_feature_probe_v064" positional \
  >"$out/feature_positional.log" 2>&1
feature_positional_rc=$?
set -e
test "$feature_positional_rc" -eq 2
grep -q 'unknown or incomplete option' "$out/feature_positional.log"
echo 'snapshot_reads=0' >"$out/feature_probe_io_contract.txt"

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

make -C "$source_copy" stellar_camera_director_v060 \
  test_stellar_cinematic_director_v060 \
  >"$out/cinematic_v060_make.log" 2>&1
test -x "$source_copy/stellar_camera_director_v060"
test -x "$source_copy/test_stellar_cinematic_director_v060"
"$source_copy/test_stellar_cinematic_director_v060" \
  >"$out/cinematic_v060_test.log" 2>&1
grep -q '^STELLAR_CINEMATIC_DIRECTOR_V060_OK$' \
  "$out/cinematic_v060_test.log"

cp "$out/v059_landmarks.tsv" "$out/v060_landmarks.tsv"
cat >"$out/v060_shots.tsv" <<'EOF_V060_SHOTS'
# schema=stellar_cinematic_shots_v060
approach 100 102 orbit binary 10 20 1.10 30 - 1 filtered - linear
disk_reveal 103 104 disk_oblique disk 35 38 1.06 - - 1 raw 104 smoothstep
north_lobe 105 106 outflow_follow positive_lobe 45 8 1.12 - - 1 raw 106 smootherstep
EOF_V060_SHOTS

run_v060_loose() {
  local directory="$1"
  mkdir -p "$directory"
  (cd "$directory" && "$source_copy/stellar_camera_director_v060" \
    --landmarks "$out/v060_landmarks.tsv" \
    --shots "$out/v060_shots.tsv" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1000 \
    --max-final-target-scale-error-percent 0) \
    >"$directory/director.log" 2>&1
  grep -q 'STELLAR_CAMERA_DIRECTOR_V060_OK rows=7 shots=3 dry_run=false' \
    "$directory/director.log"
}
run_v060_loose "$out/v060_run1"
run_v060_loose "$out/v060_run2"
for product in camera.tsv diagnostics.tsv manifest.txt preview.svg; do
  cmp "$out/v060_run1/$product" "$out/v060_run2/$product"
done
grep -q '^# director_schema=stellar_cinematic_director_v060$' \
  "$out/v060_run1/camera.tsv"
grep -q '^# schema=stellar_camera_diagnostics_v060$' \
  "$out/v060_run1/diagnostics.tsv"
grep -q 'target_scale_error_percent scale_limited' \
  "$out/v060_run1/diagnostics.tsv"
grep -q 'raw_material_occupancy raw_disk_occupancy raw_outflow_occupancy' \
  "$out/v060_run1/diagnostics.tsv"
grep -q '^schema=stellar_cinematic_direction_manifest_v060$' \
  "$out/v060_run1/manifest.txt"
grep -q '^framing.extent_selection=per_shot_raw_or_filtered$' \
  "$out/v060_run1/manifest.txt"
grep -q '^framing.scale_limiter=path_log_step$' \
  "$out/v060_run1/manifest.txt"
grep -q '^shot.0.extent_source=filtered$' "$out/v060_run1/manifest.txt"
grep -q '^shot.1.extent_source=raw$' "$out/v060_run1/manifest.txt"
grep -q '^motion.final_target_scale_error_percent=0$' \
  "$out/v060_run1/manifest.txt"
grep -q '^motion_budget.passed=true$' "$out/v060_run1/manifest.txt"
grep -q '^path_written=true$' "$out/v060_run1/manifest.txt"
"$source_copy/build/test_stellar_camera_path_v055" \
  "$out/v060_run1/camera.tsv" >"$out/v060_native_path_test.log" 2>&1
grep -q STELLAR_CAMERA_PATH_V055_OK "$out/v060_native_path_test.log"

mkdir -p "$out/v060_limited"
(cd "$out/v060_limited" && "$source_copy/stellar_camera_director_v060" \
  --landmarks "$out/v060_landmarks.tsv" \
  --shots "$out/v060_shots.tsv" \
  --path camera.tsv \
  --diagnostics diagnostics.tsv \
  --manifest manifest.txt \
  --preview preview.svg \
  --minimum-half-extent-cm 1 \
  --max-roll-deg-per-frame 180 \
  --max-zoom-percent-per-frame 1 \
  --max-final-target-scale-error-percent 1e9) \
  >"$out/v060_limited/director.log" 2>&1
awk '
  !/^#/ {
    if ($17 > 1.0000000001) exit 1
    limited += ($20 == 1)
  }
  END {if (limited == 0) exit 2}
' "$out/v060_limited/diagnostics.tsv"
grep -Eq '^motion.scale_limited_rows=[1-9][0-9]*$' \
  "$out/v060_limited/manifest.txt"
grep -q '^motion_budget.passed=true$' "$out/v060_limited/manifest.txt"

mkdir -p "$out/v060_dry_rejected"
(cd "$out/v060_dry_rejected" && \
  "$source_copy/stellar_camera_director_v060" \
    --landmarks "$out/v060_landmarks.tsv" \
    --shots "$out/v060_shots.tsv" \
    --dry-run \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1 \
    --max-final-target-scale-error-percent 0) \
    >"$out/v060_dry_rejected/director.log" 2>&1
grep -q '^motion_budget.passed=false$' \
  "$out/v060_dry_rejected/manifest.txt"
grep -q '^path_written=false$' "$out/v060_dry_rejected/manifest.txt"
test -s "$out/v060_dry_rejected/diagnostics.tsv"
test -s "$out/v060_dry_rejected/preview.svg"

mkdir -p "$out/v060_production_rejected"
set +e
(cd "$out/v060_production_rejected" && \
  "$source_copy/stellar_camera_director_v060" \
    --landmarks "$out/v060_landmarks.tsv" \
    --shots "$out/v060_shots.tsv" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1 \
    --max-final-target-scale-error-percent 0) \
    >"$out/v060_production_rejected/director.log" 2>&1
v060_rejected_rc=$?
set -e
test "$v060_rejected_rc" -eq 2
grep -q '^STELLAR_CAMERA_DIRECTOR_V060_MOTION_REJECTED ' \
  "$out/v060_production_rejected/director.log"
grep -q ' final_target_scale_error_percent=' \
  "$out/v060_production_rejected/director.log"
test ! -e "$out/v060_production_rejected/camera.tsv"
test -s "$out/v060_production_rejected/diagnostics.tsv"
test -s "$out/v060_production_rejected/manifest.txt"
test -s "$out/v060_production_rejected/preview.svg"
grep -q '^motion_budget.passed=false$' \
  "$out/v060_production_rejected/manifest.txt"
grep -q '^path_written=false$' \
  "$out/v060_production_rejected/manifest.txt"

sha256sum "$out/v060_run1/camera.tsv" \
  "$out/v060_run1/diagnostics.tsv" \
  "$out/v060_run1/manifest.txt" \
  "$out/v060_run1/preview.svg" >"$out/v060_before_clobber.sha256"
set +e
run_v060_loose "$out/v060_run1" >"$out/v060_no_clobber_outer.log" 2>&1
v060_clobber_rc=$?
set -e
test "$v060_clobber_rc" -ne 0
grep -q 'Refusing to overwrite' "$out/v060_run1/director.log"
sha256sum "$out/v060_run1/camera.tsv" \
  "$out/v060_run1/diagnostics.tsv" \
  "$out/v060_run1/manifest.txt" \
  "$out/v060_run1/preview.svg" >"$out/v060_after_clobber.sha256"
cmp "$out/v060_before_clobber.sha256" "$out/v060_after_clobber.sha256"

cat >"$out/v060_invalid_extent.tsv" <<'EOF_V060_INVALID_EXTENT'
# schema=stellar_cinematic_shots_v060
invalid 100 106 disk_oblique disk 0 30 1.1 - - 1 stale - linear
EOF_V060_INVALID_EXTENT
mkdir -p "$out/v060_invalid_extent"
if (cd "$out/v060_invalid_extent" && \
    "$source_copy/stellar_camera_director_v060" \
      --landmarks "$out/v060_landmarks.tsv" \
      --shots "$out/v060_invalid_extent.tsv" \
      --dry-run \
      --diagnostics diagnostics.tsv \
      --manifest manifest.txt \
      --preview preview.svg) \
      >"$out/v060_invalid_extent/director.log" 2>&1; then
  echo "v060 director accepted an invalid extent source" >&2
  exit 17
fi
grep -q 'Invalid shot value' "$out/v060_invalid_extent/director.log"
test ! -e "$out/v060_invalid_extent/diagnostics.tsv"
test ! -e "$out/v060_invalid_extent/manifest.txt"
test ! -e "$out/v060_invalid_extent/preview.svg"

mkdir -p "$out/v060_invalid_numeric"
if (cd "$out/v060_invalid_numeric" && \
    "$source_copy/stellar_camera_director_v060" \
      --landmarks "$out/v060_landmarks.tsv" \
      --shots "$out/v060_shots.tsv" \
      --dry-run \
      --diagnostics diagnostics.tsv \
      --manifest manifest.txt \
      --preview preview.svg \
      --max-final-target-scale-error-percent invalid) \
      >"$out/v060_invalid_numeric/director.log" 2>&1; then
  echo "v060 director accepted invalid final target error" >&2
  exit 18
fi
grep -q 'Invalid --max-final-target-scale-error-percent' \
  "$out/v060_invalid_numeric/director.log"
test ! -e "$out/v060_invalid_numeric/diagnostics.tsv"
test ! -e "$out/v060_invalid_numeric/manifest.txt"
test ! -e "$out/v060_invalid_numeric/preview.svg"

make -C "$source_copy" stellar_camera_director_v061 \
  test_stellar_cinematic_director_v061 \
  >"$out/cinematic_v061_retained_make.log" 2>&1
test -x "$source_copy/stellar_camera_director_v061"
test -x "$source_copy/test_stellar_cinematic_director_v061"
"$source_copy/test_stellar_cinematic_director_v061" \
  >"$out/cinematic_v061_retained_test.log" 2>&1
grep -q '^STELLAR_CINEMATIC_DIRECTOR_V061_OK$' \
  "$out/cinematic_v061_retained_test.log"

make -C "$source_copy" stellar_camera_director_v062 \
  test_stellar_cinematic_director_v062 \
  >"$out/cinematic_v062_make.log" 2>&1
test -x "$source_copy/stellar_camera_director_v062"
test -x "$source_copy/test_stellar_cinematic_director_v062"
"$source_copy/test_stellar_cinematic_director_v062" \
  >"$out/cinematic_v062_test.log" 2>&1
grep -q '^STELLAR_CINEMATIC_DIRECTOR_V062_OK$' \
  "$out/cinematic_v062_test.log"

cp "$out/v060_landmarks.tsv" "$out/v062_landmarks.tsv"
cat >"$out/v062_shots.tsv" <<'EOF_V062_SHOTS'
# schema=stellar_cinematic_shots_v062
approach 100 102 orbit binary 10 20 1.10 30 - 1 filtered 60 none - -
disk_reveal 103 104 disk_oblique disk 35 38 1.06 - - 1 raw - continuous 104 smoothstep
north_lobe 105 106 outflow_follow positive_lobe 45 8 1.12 - - 1 raw - cut - -
EOF_V062_SHOTS
cat >"$out/v062_visual.tsv" <<'EOF_V062_VISUAL'
# schema=stellar_visual_framing_calibration_v062
100 approach perceptual_area 0.739 0.45 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
102 approach perceptual_area 0.36 0.45 ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff
103 disk_reveal perceptual_area 1.0 0.45 bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb
104 disk_reveal perceptual_area 0.81 0.45 cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
105 north_lobe largest_component_area 0.078 0.25 dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd
106 north_lobe largest_component_area 0.023 0.25 eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
EOF_V062_VISUAL

run_v062_loose() {
  local directory="$1"
  mkdir -p "$directory"
  (cd "$directory" && "$source_copy/stellar_camera_director_v062" \
    --landmarks "$out/v062_landmarks.tsv" \
    --shots "$out/v062_shots.tsv" \
    --visual-calibration "$out/v062_visual.tsv" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1000 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1000 \
    --max-final-target-scale-error-percent 0) \
    >"$directory/director.log" 2>&1
  grep -q 'STELLAR_CAMERA_DIRECTOR_V062_OK rows=7 shots=3 visual_calibration_rows=6 dry_run=false' \
    "$directory/director.log"
}
run_v062_loose "$out/v062_run1"
run_v062_loose "$out/v062_run2"
for product in camera.tsv diagnostics.tsv manifest.txt preview.svg; do
  cmp "$out/v062_run1/$product" "$out/v062_run2/$product"
done
grep -q '^# director_schema=stellar_cinematic_director_v062$' \
  "$out/v062_run1/camera.tsv"
grep -q '^# schema=stellar_camera_diagnostics_v062$' \
  "$out/v062_run1/diagnostics.tsv"
grep -q 'transition_fraction cut_from_previous' \
  "$out/v062_run1/diagnostics.tsv"
grep -q 'required_zoom_to_target_percent' "$out/v062_run1/diagnostics.tsv"
grep -q 'visual_scale_factor visual_target_fill' \
  "$out/v062_run1/diagnostics.tsv"
grep -q '^schema=stellar_cinematic_direction_manifest_v062$' \
  "$out/v062_run1/manifest.txt"
grep -q '^framing.raw_extents_ignore_filtered_minimum=true$' \
  "$out/v062_run1/manifest.txt"
grep -q '^framing.scale_override=direct_screen_half_extent_cm$' \
  "$out/v062_run1/manifest.txt"
grep -q '^framing.visual_calibration.source_hash_bound=true$' \
  "$out/v062_run1/manifest.txt"
grep -q '^visual_calibration_rows=6$' "$out/v062_run1/manifest.txt"
grep -q '^visual_calibration.4.metric=largest_component_area$' \
  "$out/v062_run1/manifest.txt"
grep -q '^transitions.declared_kinds=continuous,cut$' \
  "$out/v062_run1/manifest.txt"
grep -q '^shot.0.screen_half_extent_override_cm=60$' \
  "$out/v062_run1/manifest.txt"
grep -q '^shot.1.transition_kind=continuous$' \
  "$out/v062_run1/manifest.txt"
grep -q '^shot.2.transition_kind=cut$' "$out/v062_run1/manifest.txt"
grep -q '^motion.declared_cut_count=1$' "$out/v062_run1/manifest.txt"
grep -q '^motion_budget.passed=true$' "$out/v062_run1/manifest.txt"
grep -q '^path_written=true$' "$out/v062_run1/manifest.txt"
awk '
  !/^#/ && $1 == 100 {
    expected = 60 * sqrt(0.739 / 0.45)
    if (($12 - expected > 1e-10) || (expected - $12 > 1e-10)) exit 1
    anchor = 1
  }
  !/^#/ && $1 == 104 {
    expected = 1.06 * ((0.80 * 1.20 * 14 > 1.30 * 11) ?
                       0.80 * 1.20 * 14 : 1.30 * 11) * sqrt(0.81 / 0.45)
    if (($12 - expected > 1e-10) || (expected - $12 > 1e-10)) exit 2
    raw = 1
  }
  END {if (!anchor || !raw) exit 3}
' "$out/v062_run1/camera.tsv"
awk '
  !/^#/ && $1 == 105 {
    if ($7 != 1 || $18 != 0 || !($19 > 0)) exit 1
    cut = 1
  }
  END {if (!cut) exit 2}
' "$out/v062_run1/diagnostics.tsv"
"$source_copy/build/test_stellar_camera_path_v055" \
  "$out/v062_run1/camera.tsv" >"$out/v062_native_path_test.log" 2>&1
grep -q STELLAR_CAMERA_PATH_V055_OK "$out/v062_native_path_test.log"

mkdir -p "$out/v062_limited"
(cd "$out/v062_limited" && "$source_copy/stellar_camera_director_v062" \
  --landmarks "$out/v062_landmarks.tsv" \
  --shots "$out/v062_shots.tsv" \
  --visual-calibration "$out/v062_visual.tsv" \
  --path camera.tsv \
  --diagnostics diagnostics.tsv \
  --manifest manifest.txt \
  --preview preview.svg \
  --minimum-half-extent-cm 1000 \
  --max-roll-deg-per-frame 180 \
  --max-zoom-percent-per-frame 1 \
  --max-final-target-scale-error-percent 1e9) \
  >"$out/v062_limited/director.log" 2>&1
awk '
  !/^#/ {
    if ($18 > 1.0000000001) exit 1
    limited += ($22 == 1)
  }
  END {if (limited == 0) exit 2}
' "$out/v062_limited/diagnostics.tsv"
grep -Eq '^motion.scale_limited_rows=[1-9][0-9]*$' \
  "$out/v062_limited/manifest.txt"
grep -q '^motion.declared_cut_count=1$' "$out/v062_limited/manifest.txt"
grep -q '^motion_budget.passed=true$' "$out/v062_limited/manifest.txt"

mkdir -p "$out/v062_production_rejected"
set +e
(cd "$out/v062_production_rejected" && \
  "$source_copy/stellar_camera_director_v062" \
    --landmarks "$out/v062_landmarks.tsv" \
    --shots "$out/v062_shots.tsv" \
    --visual-calibration "$out/v062_visual.tsv" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1000 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1 \
    --max-final-target-scale-error-percent 0) \
    >"$out/v062_production_rejected/director.log" 2>&1
v062_rejected_rc=$?
set -e
test "$v062_rejected_rc" -eq 2
grep -q '^STELLAR_CAMERA_DIRECTOR_V062_MOTION_REJECTED ' \
  "$out/v062_production_rejected/director.log"
test ! -e "$out/v062_production_rejected/camera.tsv"
test -s "$out/v062_production_rejected/diagnostics.tsv"
test -s "$out/v062_production_rejected/manifest.txt"
test -s "$out/v062_production_rejected/preview.svg"
grep -q '^motion_budget.passed=false$' \
  "$out/v062_production_rejected/manifest.txt"
grep -q '^path_written=false$' \
  "$out/v062_production_rejected/manifest.txt"

sha256sum "$out/v062_run1/camera.tsv" \
  "$out/v062_run1/diagnostics.tsv" \
  "$out/v062_run1/manifest.txt" \
  "$out/v062_run1/preview.svg" >"$out/v062_before_clobber.sha256"
set +e
run_v062_loose "$out/v062_run1" >"$out/v062_no_clobber_outer.log" 2>&1
v062_clobber_rc=$?
set -e
test "$v062_clobber_rc" -ne 0
grep -q 'Refusing to overwrite' "$out/v062_run1/director.log"
sha256sum "$out/v062_run1/camera.tsv" \
  "$out/v062_run1/diagnostics.tsv" \
  "$out/v062_run1/manifest.txt" \
  "$out/v062_run1/preview.svg" >"$out/v062_after_clobber.sha256"
cmp "$out/v062_before_clobber.sha256" "$out/v062_after_clobber.sha256"

mkdir -p "$out/v062_missing_visual"
set +e
(cd "$out/v062_missing_visual" && \
  "$source_copy/stellar_camera_director_v062" \
    --landmarks "$out/v062_landmarks.tsv" \
    --shots "$out/v062_shots.tsv" \
    --dry-run \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg) \
    >"$out/v062_missing_visual/director.log" 2>&1
v062_missing_visual_rc=$?
set -e
test "$v062_missing_visual_rc" -eq 1
grep -q 'Missing required option --visual-calibration' \
  "$out/v062_missing_visual/director.log"
test ! -e "$out/v062_missing_visual/diagnostics.tsv"
test ! -e "$out/v062_missing_visual/manifest.txt"
test ! -e "$out/v062_missing_visual/preview.svg"

make -C "$source_copy" stellar_camera_director_v063 \
  test_stellar_cinematic_director_v063 \
  >"$out/cinematic_v063_make.log" 2>&1
test -x "$source_copy/stellar_camera_director_v063"
test -x "$source_copy/test_stellar_cinematic_director_v063"
"$source_copy/test_stellar_cinematic_director_v063" \
  >"$out/cinematic_v063_test.log" 2>&1
grep -q '^STELLAR_CINEMATIC_DIRECTOR_V063_OK$' \
  "$out/cinematic_v063_test.log"

cp "$out/v062_landmarks.tsv" "$out/v063_landmarks.tsv"
sed 's/stellar_cinematic_shots_v062/stellar_cinematic_shots_v063/' \
  "$out/v062_shots.tsv" >"$out/v063_shots.tsv"
cat >"$out/v063_visual.tsv" <<'EOF_V063_VISUAL'
# schema=stellar_visual_framing_calibration_v063
100 approach perceptual_area 0.739 0.45 75 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa 1111111111111111111111111111111111111111111111111111111111111111
102 approach perceptual_area 0.36 0.45 90 ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff 1111111111111111111111111111111111111111111111111111111111111111
103 disk_reveal perceptual_area 1.0 0.45 30 bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb 2222222222222222222222222222222222222222222222222222222222222222
104 disk_reveal perceptual_area 0.81 0.45 45 cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc 2222222222222222222222222222222222222222222222222222222222222222
105 north_lobe largest_component_bbox 0.78 0.85 55 dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd 3333333333333333333333333333333333333333333333333333333333333333
106 north_lobe largest_component_bbox 0.64 0.85 70 eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee 3333333333333333333333333333333333333333333333333333333333333333
EOF_V063_VISUAL

run_v063_loose() {
  local directory="$1"
  local visual="${2:-$out/v063_visual.tsv}"
  mkdir -p "$directory"
  (cd "$directory" && "$source_copy/stellar_camera_director_v063" \
    --landmarks "$out/v063_landmarks.tsv" \
    --shots "$out/v063_shots.tsv" \
    --visual-calibration "$visual" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1000 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1000 \
    --max-final-target-scale-error-percent 0) \
    >"$directory/director.log" 2>&1
  grep -q 'STELLAR_CAMERA_DIRECTOR_V063_OK rows=7 shots=3 visual_calibration_rows=6 dry_run=false' \
    "$directory/director.log"
}
run_v063_loose "$out/v063_run1"
run_v063_loose "$out/v063_run2"
for product in camera.tsv diagnostics.tsv manifest.txt preview.svg; do
  cmp "$out/v063_run1/$product" "$out/v063_run2/$product"
done
grep -q '^# director_schema=stellar_cinematic_director_v063$' \
  "$out/v063_run1/camera.tsv"
grep -q '^# schema=stellar_camera_diagnostics_v063$' \
  "$out/v063_run1/diagnostics.tsv"
grep -q 'visual_source_screen_half_extent_cm visual_absolute_target_half_extent_cm' \
  "$out/v063_run1/diagnostics.tsv"
grep -q '^schema=stellar_cinematic_direction_manifest_v063$' \
  "$out/v063_run1/manifest.txt"
grep -q '^framing.visual_calibration.absolute_target_rebased=true$' \
  "$out/v063_run1/manifest.txt"
grep -q '^framing.visual_calibration.source_frame_hash_bound=true$' \
  "$out/v063_run1/manifest.txt"
grep -q '^framing.visual_calibration.source_camera_path_hash_bound=true$' \
  "$out/v063_run1/manifest.txt"
grep -q '^visual_calibration_rows=6$' "$out/v063_run1/manifest.txt"
grep -q '^visual_calibration.0.source_screen_half_extent_cm=75$' \
  "$out/v063_run1/manifest.txt"
grep -q '^visual_calibration.0.source_camera_path_sha256=11111111' \
  "$out/v063_run1/manifest.txt"
grep -q '^visual_calibration.4.metric=largest_component_bbox$' \
  "$out/v063_run1/manifest.txt"
grep -q '^motion.declared_cut_count=1$' "$out/v063_run1/manifest.txt"
grep -q '^motion_budget.passed=true$' "$out/v063_run1/manifest.txt"
grep -q '^path_written=true$' "$out/v063_run1/manifest.txt"
awk '
  !/^#/ && $1 == 100 {
    expected = 75 * sqrt(0.739 / 0.45)
    if (($12 - expected > 1e-10) || (expected - $12 > 1e-10)) exit 1
    opening = 1
  }
  !/^#/ && $1 == 104 {
    expected = 45 * sqrt(0.81 / 0.45)
    if (($12 - expected > 1e-10) || (expected - $12 > 1e-10)) exit 2
    disk = 1
  }
  !/^#/ && $1 == 106 {
    expected = 70 * (0.64 / 0.85)
    if (($12 - expected > 1e-10) || (expected - $12 > 1e-10)) exit 3
    outflow = 1
  }
  END {if (!opening || !disk || !outflow) exit 4}
' "$out/v063_run1/camera.tsv"
"$source_copy/build/test_stellar_camera_path_v055" \
  "$out/v063_run1/camera.tsv" >"$out/v063_native_path_test.log" 2>&1
grep -q STELLAR_CAMERA_PATH_V055_OK "$out/v063_native_path_test.log"

v063_source_path_sha=$(sha256sum "$out/v063_run1/camera.tsv" | awk '{print $1}')
awk -v path_sha="$v063_source_path_sha" '
  NR == FNR {
    if ($0 !~ /^#/) half[$1] = $12
    next
  }
  /^#/ {
    print "# schema=stellar_visual_framing_calibration_v063"
    next
  }
  {
    print $1, $2, $3, $4, $5, half[$1], $7, path_sha
  }
' "$out/v063_run1/camera.tsv" "$out/v063_visual.tsv" \
  >"$out/v063_iterative_visual.tsv"
run_v063_loose "$out/v063_iterative" "$out/v063_iterative_visual.tsv"
awk '
  NR == FNR {
    if ($0 !~ /^#/) first[$1] = $12
    next
  }
  !/^#/ && $1 == 100 {
    expected = first[$1] * sqrt(0.739 / 0.45)
    if (($12 - expected > 1e-10) || (expected - $12 > 1e-10)) exit 1
    if (($12 - first[$1] < 1e-10) && (first[$1] - $12 < 1e-10)) exit 2
    compounded = 1
  }
  END {if (!compounded) exit 3}
' "$out/v063_run1/camera.tsv" "$out/v063_iterative/camera.tsv"
grep -q "^visual_calibration.0.source_camera_path_sha256=$v063_source_path_sha$" \
  "$out/v063_iterative/manifest.txt"
grep -q '^framing.visual_calibration.absolute_target_rebased=true$' \
  "$out/v063_iterative/manifest.txt"

mkdir -p "$out/v063_limited"
(cd "$out/v063_limited" && "$source_copy/stellar_camera_director_v063" \
  --landmarks "$out/v063_landmarks.tsv" \
  --shots "$out/v063_shots.tsv" \
  --visual-calibration "$out/v063_visual.tsv" \
  --path camera.tsv \
  --diagnostics diagnostics.tsv \
  --manifest manifest.txt \
  --preview preview.svg \
  --minimum-half-extent-cm 1000 \
  --max-roll-deg-per-frame 180 \
  --max-zoom-percent-per-frame 1 \
  --max-final-target-scale-error-percent 1e9) \
  >"$out/v063_limited/director.log" 2>&1
awk '
  !/^#/ {
    if ($18 > 1.0000000001) exit 1
    limited += ($22 == 1)
  }
  END {if (limited == 0) exit 2}
' "$out/v063_limited/diagnostics.tsv"
grep -Eq '^motion.scale_limited_rows=[1-9][0-9]*$' \
  "$out/v063_limited/manifest.txt"
grep -q '^motion_budget.passed=true$' "$out/v063_limited/manifest.txt"

mkdir -p "$out/v063_production_rejected"
set +e
(cd "$out/v063_production_rejected" && \
  "$source_copy/stellar_camera_director_v063" \
    --landmarks "$out/v063_landmarks.tsv" \
    --shots "$out/v063_shots.tsv" \
    --visual-calibration "$out/v063_visual.tsv" \
    --path camera.tsv \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg \
    --minimum-half-extent-cm 1000 \
    --max-roll-deg-per-frame 180 \
    --max-zoom-percent-per-frame 1 \
    --max-final-target-scale-error-percent 0) \
    >"$out/v063_production_rejected/director.log" 2>&1
v063_rejected_rc=$?
set -e
test "$v063_rejected_rc" -eq 2
grep -q '^STELLAR_CAMERA_DIRECTOR_V063_MOTION_REJECTED ' \
  "$out/v063_production_rejected/director.log"
test ! -e "$out/v063_production_rejected/camera.tsv"
grep -q '^motion_budget.passed=false$' \
  "$out/v063_production_rejected/manifest.txt"
grep -q '^path_written=false$' \
  "$out/v063_production_rejected/manifest.txt"

sha256sum "$out/v063_run1/camera.tsv" \
  "$out/v063_run1/diagnostics.tsv" \
  "$out/v063_run1/manifest.txt" \
  "$out/v063_run1/preview.svg" >"$out/v063_before_clobber.sha256"
set +e
run_v063_loose "$out/v063_run1" >"$out/v063_no_clobber_outer.log" 2>&1
v063_clobber_rc=$?
set -e
test "$v063_clobber_rc" -ne 0
grep -q 'Refusing to overwrite' "$out/v063_run1/director.log"
sha256sum "$out/v063_run1/camera.tsv" \
  "$out/v063_run1/diagnostics.tsv" \
  "$out/v063_run1/manifest.txt" \
  "$out/v063_run1/preview.svg" >"$out/v063_after_clobber.sha256"
cmp "$out/v063_before_clobber.sha256" "$out/v063_after_clobber.sha256"

mkdir -p "$out/v063_missing_visual"
set +e
(cd "$out/v063_missing_visual" && \
  "$source_copy/stellar_camera_director_v063" \
    --landmarks "$out/v063_landmarks.tsv" \
    --shots "$out/v063_shots.tsv" \
    --dry-run \
    --diagnostics diagnostics.tsv \
    --manifest manifest.txt \
    --preview preview.svg) \
    >"$out/v063_missing_visual/director.log" 2>&1
v063_missing_visual_rc=$?
set -e
test "$v063_missing_visual_rc" -eq 1
grep -q 'Missing required option --visual-calibration' \
  "$out/v063_missing_visual/director.log"
test ! -e "$out/v063_missing_visual/diagnostics.tsv"
test ! -e "$out/v063_missing_visual/manifest.txt"
test ! -e "$out/v063_missing_visual/preview.svg"

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
grep -v '^#' "$out/native_camera_path.tsv" >"$out/native_camera_v056.data"
grep -v '^#' "$out/native_camera_path_v059.tsv" >"$out/native_camera_v059.data"
cmp "$out/native_camera_v056.data" "$out/native_camera_v059.data"
cat >"$out/native_shots_v060.tsv" <<'EOF_NATIVE_SHOTS_V060'
# schema=stellar_cinematic_shots_v060
native_pose 0 0 disk_edge grid 0 0 1 - - 1 filtered - linear
EOF_NATIVE_SHOTS_V060
"$source_copy/stellar_camera_director_v060" \
  --landmarks "$out/native_landmarks.tsv" \
  --shots "$out/native_shots_v060.tsv" \
  --path "$out/native_camera_path_v060.tsv" \
  --diagnostics "$out/native_camera_diagnostics_v060.tsv" \
  --manifest "$out/native_camera_manifest_v060.txt" \
  --preview "$out/native_camera_preview_v060.svg" \
  --minimum-half-extent-cm 0.01 \
  >"$out/native_camera_director_v060.log" 2>&1
grep -q 'STELLAR_CAMERA_DIRECTOR_V060_OK rows=1 shots=1 dry_run=false' \
  "$out/native_camera_director_v060.log"
grep -v '^#' "$out/native_camera_path_v060.tsv" >"$out/native_camera_v060.data"
cmp "$out/native_camera_v056.data" "$out/native_camera_v060.data"
cat >"$out/native_shots_v062.tsv" <<'EOF_NATIVE_SHOTS_V062'
# schema=stellar_cinematic_shots_v062
native_pose 0 0 disk_edge grid 0 0 1 - - 1 filtered - none - -
EOF_NATIVE_SHOTS_V062
cat >"$out/native_visual_v062.tsv" <<'EOF_NATIVE_VISUAL_V062'
# schema=stellar_visual_framing_calibration_v062
0 native_pose perceptual_area 0.5 0.5 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
EOF_NATIVE_VISUAL_V062
"$source_copy/stellar_camera_director_v062" \
  --landmarks "$out/native_landmarks.tsv" \
  --shots "$out/native_shots_v062.tsv" \
  --visual-calibration "$out/native_visual_v062.tsv" \
  --path "$out/native_camera_path_v062.tsv" \
  --diagnostics "$out/native_camera_diagnostics_v062.tsv" \
  --manifest "$out/native_camera_manifest_v062.txt" \
  --preview "$out/native_camera_preview_v062.svg" \
  --minimum-half-extent-cm 0.01 \
  >"$out/native_camera_director_v062.log" 2>&1
grep -q 'STELLAR_CAMERA_DIRECTOR_V062_OK rows=1 shots=1 visual_calibration_rows=1 dry_run=false' \
  "$out/native_camera_director_v062.log"
grep -v '^#' "$out/native_camera_path_v062.tsv" >"$out/native_camera_v062.data"
cmp "$out/native_camera_v056.data" "$out/native_camera_v062.data"
cat >"$out/native_shots_v063.tsv" <<'EOF_NATIVE_SHOTS_V063'
# schema=stellar_cinematic_shots_v063
native_pose 0 0 disk_edge grid 0 0 1 - - 1 filtered - none - -
EOF_NATIVE_SHOTS_V063
native_v056_path_sha=$(sha256sum "$out/native_camera_path.tsv" | awk '{print $1}')
printf '%s\n%s\n' \
  '# schema=stellar_visual_framing_calibration_v063' \
  "0 native_pose perceptual_area 0.5 0.5 0.52 aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa $native_v056_path_sha" \
  >"$out/native_visual_v063.tsv"
"$source_copy/stellar_camera_director_v063" \
  --landmarks "$out/native_landmarks.tsv" \
  --shots "$out/native_shots_v063.tsv" \
  --visual-calibration "$out/native_visual_v063.tsv" \
  --path "$out/native_camera_path_v063.tsv" \
  --diagnostics "$out/native_camera_diagnostics_v063.tsv" \
  --manifest "$out/native_camera_manifest_v063.txt" \
  --preview "$out/native_camera_preview_v063.svg" \
  --minimum-half-extent-cm 0.01 \
  >"$out/native_camera_director_v063.log" 2>&1
grep -q 'STELLAR_CAMERA_DIRECTOR_V063_OK rows=1 shots=1 visual_calibration_rows=1 dry_run=false' \
  "$out/native_camera_director_v063.log"
grep -q '^framing.visual_calibration.absolute_target_rebased=true$' \
  "$out/native_camera_manifest_v063.txt"
grep -q "^visual_calibration.0.source_camera_path_sha256=$native_v056_path_sha$" \
  "$out/native_camera_manifest_v063.txt"
grep -v '^#' "$out/native_camera_path_v063.tsv" >"$out/native_camera_v063.data"
cmp "$out/native_camera_v056.data" "$out/native_camera_v063.data"
"$source_copy/build/test_stellar_camera_path_v055" \
  "$out/native_camera_path_v063.tsv" >"$out/native_v063_path_test.log" 2>&1
grep -q STELLAR_CAMERA_PATH_V055_OK "$out/native_v063_path_test.log"

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
awk -v output="phase11_native_camera.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  {print}
' "$source_copy/tests/config_phase7_decoy_camera.txt" \
  >"$source_copy/tests/config_phase11_native_camera.txt"
{
  echo "stellarCameraPath = $out/native_camera_path_v060.tsv"
  echo "stellarCameraDirectionManifest = $out/native_camera_manifest_v060.txt"
} >>"$source_copy/tests/config_phase11_native_camera.txt"
awk -v output="phase12_native_camera.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  {print}
' "$source_copy/tests/config_phase7_decoy_camera.txt" \
  >"$source_copy/tests/config_phase12_native_camera.txt"
{
  echo "stellarCameraPath = $out/native_camera_path_v062.tsv"
  echo "stellarCameraDirectionManifest = $out/native_camera_manifest_v062.txt"
} >>"$source_copy/tests/config_phase12_native_camera.txt"
awk -v output="phase14_native_camera.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  {print}
' "$source_copy/tests/config_phase7_decoy_camera.txt" \
  >"$source_copy/tests/config_phase14_native_camera.txt"
{
  echo "stellarCameraPath = $out/native_camera_path_v063.tsv"
  echo "stellarCameraDirectionManifest = $out/native_camera_manifest_v063.txt"
} >>"$source_copy/tests/config_phase14_native_camera.txt"
awk -v output="phase17_native_feature.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  {print}
' "$source_copy/tests/config_palette_accent_run1.txt" \
  >"$source_copy/tests/config_phase17_native_feature.txt"
{
  echo "stellarCameraPath = $out/native_camera_path_v063.tsv"
  echo "stellarCameraDirectionManifest = $out/native_camera_manifest_v063.txt"
  echo "stellarFeatureProfile = stellar_structures_v065"
} >>"$source_copy/tests/config_phase17_native_feature.txt"
awk -v output="phase17_invalid_feature.png" '
  $1 == "imageFile" {print "imageFile = " output; next}
  {print}
' "$source_copy/tests/config_palette_accent_run1.txt" \
  >"$source_copy/tests/config_phase17_invalid_feature.txt"
{
  echo "stellarCameraPath = $out/native_camera_path_v063.tsv"
  echo "stellarCameraDirectionManifest = $out/native_camera_manifest_v063.txt"
  echo "stellarFeatureProfile = invalid"
} >>"$source_copy/tests/config_phase17_invalid_feature.txt"
rm -f "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase7_decoy_camera.png" \
  "$source_copy/phase7_native_camera.png" \
  "$source_copy/phase10_native_camera.png" \
  "$source_copy/phase11_native_camera.png" \
  "$source_copy/phase12_native_camera.png" \
  "$source_copy/phase14_native_camera.png" \
  "$source_copy/phase17_native_feature.png" \
  "$source_copy/phase17_invalid_feature.png"
(cd "$source_copy" && ./ArepoRT tests/config_phase7_explicit_camera.txt) \
  >"$out/native_explicit_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase7_decoy_camera.txt) \
  >"$out/native_decoy_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase7_native_camera.txt) \
  >"$out/native_manifest_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase10_native_camera.txt) \
  >"$out/native_manifest_v059_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase11_native_camera.txt) \
  >"$out/native_manifest_v060_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase12_native_camera.txt) \
  >"$out/native_manifest_v062_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase14_native_camera.txt) \
  >"$out/native_manifest_v063_render.log" 2>&1
(cd "$source_copy" && ./ArepoRT tests/config_phase17_native_feature.txt) \
  >"$out/native_feature_v065_render.log" 2>&1
set +e
(cd "$source_copy" && ./ArepoRT tests/config_phase17_invalid_feature.txt) \
  >"$out/native_feature_invalid.log" 2>&1
native_feature_invalid_rc=$?
set -e
printf 'exit_code=%s\n' "$native_feature_invalid_rc" \
  >"$out/native_feature_invalid_exit_status.txt"
test -s "$source_copy/phase7_explicit_camera.png"
test -s "$source_copy/phase7_decoy_camera.png"
test -s "$source_copy/phase7_native_camera.png"
test -s "$source_copy/phase10_native_camera.png"
test -s "$source_copy/phase11_native_camera.png"
test -s "$source_copy/phase12_native_camera.png"
test -s "$source_copy/phase14_native_camera.png"
test -s "$source_copy/phase17_native_feature.png"
test ! -e "$source_copy/phase17_invalid_feature.png"
grep -q 'Config: unknown stellarFeatureProfile.' \
  "$out/native_feature_invalid.log"
if cmp -s "$source_copy/phase7_explicit_camera.png" \
    "$source_copy/phase7_decoy_camera.png"; then
  echo "decoy camera did not change the reference image" >&2
  exit 15
fi
cmp "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase7_native_camera.png"
cmp "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase10_native_camera.png"
cmp "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase11_native_camera.png"
cmp "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase12_native_camera.png"
cmp "$source_copy/phase7_explicit_camera.png" \
  "$source_copy/phase14_native_camera.png"
grep -q '^STELLAR_CAMERA_MANIFEST_V056 .*schema=stellar_cinematic_direction_manifest_v056$' \
  "$out/native_manifest_render.log"
grep -q '^STELLAR_CAMERA_MANIFEST_V059 .*schema=stellar_cinematic_direction_manifest_v059$' \
  "$out/native_manifest_v059_render.log"
grep -q '^STELLAR_CAMERA_MANIFEST_V060 .*schema=stellar_cinematic_direction_manifest_v060$' \
  "$out/native_manifest_v060_render.log"
grep -q '^STELLAR_CAMERA_MANIFEST_V062 .*schema=stellar_cinematic_direction_manifest_v062$' \
  "$out/native_manifest_v062_render.log"
grep -q '^STELLAR_CAMERA_MANIFEST_V063 .*schema=stellar_cinematic_direction_manifest_v063$' \
  "$out/native_manifest_v063_render.log"
grep -Fq 'STELLAR_FEATURE_PROFILE_V065 profile=legacy_v064 id=0 disk_inner_fraction=0,0 disk_density_taper=0,0 disk_density_floor=0 polar_confidence=0,0 polar_envelope_floor=0' \
  "$out/palette_accent_run1.log"
grep -Fq 'STELLAR_FEATURE_PROFILE_V065 profile=stellar_structures_v065 id=1 disk_inner_fraction=0.12,0.32 disk_density_taper=2.5,4.5 disk_density_floor=0.08 polar_confidence=0.08,0.28 polar_envelope_floor=0.2' \
  "$out/native_feature_v065_render.log"
grep -q '^STELLAR_CAMERA_MANIFEST_V063 .*schema=stellar_cinematic_direction_manifest_v063$' \
  "$out/native_feature_v065_render.log"
grep -q '^STELLAR_CAMERA_PATH_V055 snapshot=0 position=' \
  "$out/native_feature_v065_render.log"
grep -q '^STELLAR_CAMERA_PATH_V055 snapshot=0 position=' \
  "$out/native_manifest_v059_render.log"
grep -q ' look_at=' "$out/native_manifest_v059_render.log"
grep -q ' up=' "$out/native_manifest_v059_render.log"
grep -q ' half_extent=' "$out/native_manifest_v059_render.log"
grep -q '^STELLAR_CAMERA_PATH_V055 snapshot=0 position=' \
  "$out/native_manifest_v060_render.log"
grep -q ' look_at=' "$out/native_manifest_v060_render.log"
grep -q ' up=' "$out/native_manifest_v060_render.log"
grep -q ' half_extent=' "$out/native_manifest_v060_render.log"
grep -q '^STELLAR_CAMERA_PATH_V055 snapshot=0 position=' \
  "$out/native_manifest_v062_render.log"
grep -q ' look_at=' "$out/native_manifest_v062_render.log"
grep -q ' up=' "$out/native_manifest_v062_render.log"
grep -q ' half_extent=' "$out/native_manifest_v062_render.log"
grep -q '^STELLAR_CAMERA_PATH_V055 snapshot=0 position=' \
  "$out/native_manifest_v063_render.log"
grep -q ' look_at=' "$out/native_manifest_v063_render.log"
grep -q ' up=' "$out/native_manifest_v063_render.log"
grep -q ' half_extent=' "$out/native_manifest_v063_render.log"
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
cp "$source_copy/phase11_native_camera.png" \
  "$out/artifacts/phase11_native_camera.png"
cp "$source_copy/phase12_native_camera.png" \
  "$out/artifacts/phase12_native_camera.png"
cp "$source_copy/phase14_native_camera.png" \
  "$out/artifacts/phase14_native_camera.png"
cp "$source_copy/phase17_native_feature.png" \
  "$out/artifacts/phase17_native_feature.png"
sha256sum "$out/artifacts/phase7_explicit_camera.png" \
  "$out/artifacts/phase7_decoy_camera.png" \
  "$out/artifacts/phase7_native_camera.png" \
  "$out/artifacts/phase10_native_camera.png" \
  "$out/artifacts/phase11_native_camera.png" \
  "$out/artifacts/phase12_native_camera.png" \
  "$out/artifacts/phase14_native_camera.png" \
  "$out/artifacts/phase17_native_feature.png" \
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
  "$source_copy/stellar_camera_director_v060" \
  "$source_copy/stellar_camera_director_v061" \
  "$source_copy/stellar_camera_director_v062" \
  "$source_copy/stellar_camera_director_v063" \
  "$source_copy/test_stellar_cinematic_director_v059" \
  "$source_copy/test_stellar_cinematic_director_v060" \
  "$source_copy/test_stellar_cinematic_director_v061" \
  "$source_copy/test_stellar_cinematic_director_v062" \
  "$source_copy/test_stellar_cinematic_director_v063" \
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
  "$source_copy/stellar_feature_probe_v064" \
  "$source_copy/test_stellar_feature_diagnostics_v064" \
  "$source_copy/test_stellar_feature_framing_v067" \
  "$source_copy/test_stellar_feature_landmarks_v066" \
  "$source_copy/test_stellar_feature_profile_v065" \
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
  "$out/v060_run1/camera.tsv" \
  "$out/v060_run1/diagnostics.tsv" \
  "$out/v060_run1/manifest.txt" \
  "$out/v060_run1/preview.svg" \
  "$out/v060_limited/camera.tsv" \
  "$out/v060_limited/diagnostics.tsv" \
  "$out/v060_limited/manifest.txt" \
  "$out/native_camera_path_v060.tsv" \
  "$out/native_camera_manifest_v060.txt" \
  "$out/v062_run1/camera.tsv" \
  "$out/v062_run1/diagnostics.tsv" \
  "$out/v062_run1/manifest.txt" \
  "$out/v062_run1/preview.svg" \
  "$out/v062_visual.tsv" \
  "$out/v062_limited/camera.tsv" \
  "$out/v062_limited/diagnostics.tsv" \
  "$out/v062_limited/manifest.txt" \
  "$out/native_camera_path_v062.tsv" \
  "$out/native_camera_manifest_v062.txt" \
  "$out/native_visual_v062.tsv" \
  "$out/v063_run1/camera.tsv" \
  "$out/v063_run1/diagnostics.tsv" \
  "$out/v063_run1/manifest.txt" \
  "$out/v063_run1/preview.svg" \
  "$out/v063_visual.tsv" \
  "$out/v063_iterative/camera.tsv" \
  "$out/v063_iterative/diagnostics.tsv" \
  "$out/v063_iterative/manifest.txt" \
  "$out/v063_iterative/preview.svg" \
  "$out/v063_iterative_visual.tsv" \
  "$out/v063_limited/camera.tsv" \
  "$out/v063_limited/diagnostics.tsv" \
  "$out/v063_limited/manifest.txt" \
  "$out/native_camera_path_v063.tsv" \
  "$out/native_camera_manifest_v063.txt" \
  "$out/native_visual_v063.tsv" \
  >"$out/cinematic_products.sha256"
sha256sum "$out/feature_fixture_run1/feature_scene_v064.bin" \
  "$out/feature_fixture_run1/feature_scene_quantized_v064.bin" \
  "$out/feature_fixture_run1/feature_report_v064.tsv" \
  "$out/feature_fixture_run1/feature_report_structures_v065.tsv" \
  "$out/feature_fixture_run1/feature_landmarks_v066.tsv" \
  "$out/feature_fixture_run1/feature_landmarks_structures_v066.tsv" \
  "$out/feature_fixture_run2/feature_scene_v064.bin" \
  "$out/feature_fixture_run2/feature_scene_quantized_v064.bin" \
  "$out/feature_fixture_run2/feature_report_v064.tsv" \
  "$out/feature_fixture_run2/feature_report_structures_v065.tsv" \
  "$out/feature_fixture_run2/feature_landmarks_v066.tsv" \
  "$out/feature_fixture_run2/feature_landmarks_structures_v066.tsv" \
  "$out/feature_cli_run1.tsv" \
  "$out/feature_cli_run2.tsv" \
  "$out/feature_structures_cli_run1.tsv" \
  "$out/feature_structures_cli_run2.tsv" \
  "$out/feature_paired_cli_run1.tsv" \
  "$out/feature_paired_cli_run2.tsv" \
  "$out/feature_landmark_cli_run1.tsv" \
  "$out/feature_landmark_cli_run2.tsv" \
  "$out/feature_framing_profile_run1.tsv" \
  "$out/feature_framing_profile_run2.tsv" \
  "$out/feature_framing_landmarks_run1.tsv" \
  "$out/feature_framing_landmarks_run2.tsv" \
  "$out/feature_framing_plan_run1.tsv" \
  "$out/feature_framing_plan_run2.tsv" \
  "$out/framing_unit/framing_plan_v067.tsv" \
  "$out/feature_framing_v067_test.log" \
  "$out/feature_quantized_cli_run1.tsv" \
  "$out/feature_quantized_cli_run2.tsv" \
  "$out/landmark_unit/landmark_fixture_v066.tsv" \
  "$out/feature_landmarks_v066_test.log" \
  "$out/feature_profile_v065_test.log" \
  "$out/feature_invalid_profile.log" \
  "$out/native_feature_v065_render.log" \
  "$out/native_feature_invalid.log" \
  "$out/native_feature_invalid_exit_status.txt" \
  "$source_copy/tests/config_phase17_native_feature.txt" \
  "$source_copy/tests/config_phase17_invalid_feature.txt" \
  "$out/feature_probe_io_contract.txt" \
  >"$out/feature_products.sha256"
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
  echo "stellar_cinematic_director_v060_tests=1"
  echo "camera_framing_extent_sources=filtered,raw"
  echo "camera_raw_extent_formulas_versioned=true"
  echo "camera_raw_occupancy_independent=true"
  echo "camera_log_scale_limiter=true"
  echo "camera_scale_limiter_budget_source=max_zoom_percent_per_frame"
  echo "camera_scale_limited_rows_reported=true"
  echo "camera_final_target_scale_error_gated=true"
  echo "camera_v060_path_native_v055_compatible=true"
  echo "camera_v060_manifest_bound_native=true"
  echo "camera_v059_manifest_behavior_retained=true"
  echo "stellar_cinematic_director_v061_tests=1"
  echo "camera_v061_manifest_behavior_retained=true"
  echo "stellar_cinematic_director_v062_tests=1"
  echo "camera_anchor_scale_override=true"
  echo "camera_raw_extents_ignore_filtered_floor=true"
  echo "camera_transition_kinds=continuous,cut"
  echo "camera_cut_motion_derivatives_reset=true"
  echo "camera_cut_required_zoom_reported=true"
  echo "camera_declared_cut_count_reported=true"
  echo "camera_v062_path_native_v055_compatible=true"
  echo "camera_v062_manifest_bound_native=true"
  echo "camera_v060_manifest_behavior_retained=true"
  echo "camera_visual_calibration_schema=stellar_visual_framing_calibration_v062"
  echo "camera_visual_calibration_required_per_shot=true"
  echo "camera_visual_calibration_source_sha256_bound=true"
  echo "camera_visual_calibration_metrics=perceptual_area,largest_component_area,perceptual_bbox,largest_component_bbox"
  echo "camera_visual_calibration_area_rule=sqrt_observed_over_target"
  echo "camera_visual_calibration_bbox_rule=observed_over_target"
  echo "camera_visual_calibration_interpolation=per_shot_log_scale_piecewise_linear_in_landmark_time"
  echo "camera_visual_calibration_cartesian_authored=false"
  echo "stellar_cinematic_director_v063_tests=1"
  echo "camera_v063_path_native_v055_compatible=true"
  echo "camera_v063_manifest_bound_native=true"
  echo "camera_v062_manifest_behavior_retained=true"
  echo "camera_iterative_calibration_schema=stellar_visual_framing_calibration_v063"
  echo "camera_iterative_calibration_source_half_extent_bound=true"
  echo "camera_iterative_calibration_source_frame_sha256_bound=true"
  echo "camera_iterative_calibration_source_path_sha256_bound=true"
  echo "camera_iterative_calibration_absolute_target_rebased=true"
  echo "camera_iterative_calibration_compounding_tested=true"
  echo "camera_iterative_calibration_interpolation=absolute_target_log_linear_in_landmark_time"
  echo "camera_outflow_calibration_metric=largest_component_bbox"
  echo "stellar_feature_diagnostics_v064_tests=1"
  echo "stellar_feature_profile_v065_tests=1"
  echo "stellar_feature_default=legacy_v064"
  echo "stellar_feature_legacy_selector_equations_changed=false"
  echo "stellar_feature_opt_in_selector_equations_changed=true"
  echo "stellar_feature_legacy_exact=true"
  echo "stellar_feature_structures_opt_in=true"
  echo "stellar_feature_disk_annular_core_suppression=true"
  echo "stellar_feature_disk_density_retention=true"
  echo "stellar_feature_polar_confidence_taper=true"
  echo "stellar_feature_native_provenance_logged=true"
  echo "stellar_feature_invalid_profile_rejected=true"
  echo "stellar_feature_invalid_profile_exit_code=$native_feature_invalid_rc"
  echo "stellar_feature_invalid_profile_exit_semantics=historical_terminate_exit_zero"
  echo "stellar_feature_gpu_contract=legacy_only"
  echo "stellar_feature_disk_merger_overlap_reported=true"
  echo "stellar_feature_disk_high_density_fraction_reported=true"
  echo "stellar_feature_disk_inner_radius_fraction_reported=true"
  echo "stellar_feature_polar_lobes_reported_separately=true"
  echo "stellar_feature_projected_cell_center_support=true"
  echo "stellar_feature_ray_grid_fit=all_active_affine_dual_basis"
  echo "stellar_feature_large_origin_quantization_test=true"
  echo "stellar_feature_ray_grid_skew_and_residual_gated=true"
  echo "stellar_feature_right_censoring_reported=true"
  echo "stellar_feature_probe_deterministic=true"
  echo "stellar_feature_probe_no_clobber=true"
  echo "stellar_feature_probe_strict_input=true"
  echo "stellar_feature_probe_snapshot_reads=0"
  echo "stellar_feature_landmarks_v066_tests=1"
  echo "stellar_feature_v064_output_exact=true"
  echo "stellar_feature_landmark_schema=stellar_feature_landmarks_v066"
  echo "stellar_feature_landmark_projection=all_selected_cell_centers_unclipped"
  echo "stellar_feature_landmark_quantiles=q90,q95,q99"
  echo "stellar_feature_landmark_screen_center=true"
  echo "stellar_feature_landmark_signed_lobes=true"
  echo "stellar_feature_landmark_raw_censoring_retained=true"
  echo "stellar_feature_landmark_outlier_resistant=true"
  echo "stellar_feature_landmark_deterministic=true"
  echo "stellar_feature_landmark_no_clobber=true"
  echo "stellar_feature_landmark_snapshot_reads=0"
  echo "stellar_feature_framing_v067_tests=1"
  echo "stellar_feature_framing_schema=stellar_feature_framing_plan_v067"
  echo "stellar_feature_framing_modes=single,bipolar_union"
  echo "stellar_feature_framing_coverages=0.90,0.98"
  echo "stellar_feature_framing_signed_bounds=true"
  echo "stellar_feature_framing_signed_lobes_separate=true"
  echo "stellar_feature_framing_union_preserves_broader_lobe=true"
  echo "stellar_feature_framing_look_at_shift_reported=true"
  echo "stellar_feature_framing_half_extent_scale_reported=true"
  echo "stellar_feature_framing_raw_span_not_radius=true"
  echo "stellar_feature_framing_raw_censoring_retained=true"
  echo "stellar_feature_framing_absent_feature_rejected=true"
  echo "stellar_feature_framing_deterministic=true"
  echo "stellar_feature_framing_no_clobber=true"
  echo "stellar_feature_framing_snapshot_reads=0"
  echo "stellar_feature_framing_camera_mutation=false"
  echo "stellar_feature_v064_output_exact_to_phase18=true"
  echo "stellar_feature_v066_output_exact_to_phase18=true"
  echo "stellar_feature_selector_equations_changed=false"
  echo "stellar_renderer_equations_changed=false"
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
touch "$out/PHASE19_FEATURE_FRAMING_VALIDATE_OK"
cat "$out/summary.txt"
echo "PHASE19_FEATURE_FRAMING_VALIDATE_OK output=$out host=$host job=$SLURM_JOB_ID"
