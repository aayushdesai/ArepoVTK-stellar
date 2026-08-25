#!/bin/bash
#SBATCH --job-name=arepovtk_stellar_p0
#SBATCH --account=caiazgrp
#SBATCH --partition=defaultp
#SBATCH --constraint=eta
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --threads-per-core=1
#SBATCH --mem=96G
#SBATCH --time=01:00:00
#SBATCH --output=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase0_validate_%j.out
#SBATCH --error=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar/validation/logs/phase0_validate_%j.err

if [[ -z "${SLURM_JOB_ID:-}" ]]; then
  echo "SLURM_JOB_ID is required" >&2
  exit 2
fi
host="$(hostname -s)"
if [[ "$host" != eta* ]]; then
  echo "Refusing AREPO work on non-eta host: $host" >&2
  exit 3
fi
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
if [[ "${CONDA_DEFAULT_ENV:-}" != "Arepo_Env" ]]; then
  echo "Arepo_Env is not active" >&2
  exit 4
fi

repo=/nfs/scistore25/caiazgrp/adesai/Arepo/ArepoVTK-stellar
dependency=/nfs/scistore25/caiazgrp/adesai/Arepo/outreach_volume_movie/v031_rework/third_party/ArepoVTK_10b14c15/arepo
reference_commit=10b14c15fd864f01fcf1f3af893f8c740163f61f
arepo_commit=d924fb39f2933129614d9a36296099b8137b2cde
out="$repo/validation/phase0_${SLURM_JOB_ID}"
source_copy="$out/source"

if [[ -e "$out" ]]; then
  echo "Refusing existing validation directory: $out" >&2
  exit 5
fi
if [[ "$(git -C "$repo" rev-parse "$reference_commit")" != "$reference_commit" ]]; then
  echo "Immutable ArepoVTK reference commit is missing" >&2
  exit 6
fi
if [[ "$(git -C "$dependency" rev-parse HEAD)" != "$arepo_commit" ]]; then
  echo "Public AREPO dependency commit changed" >&2
  exit 7
fi
if [[ -n "$(git -C "$dependency" status --porcelain)" ]]; then
  echo "Public AREPO dependency is dirty" >&2
  exit 8
fi

mkdir -p "$out" "$source_copy" "$out/artifacts/run1" "$out/artifacts/run2"

{
  echo "phase=0"
  echo "job_id=$SLURM_JOB_ID"
  echo "host=$host"
  echo "started=$(date -Is)"
  echo "repo=$repo"
  echo "branch=$(git -C "$repo" branch --show-current)"
  echo "candidate_commit=$(git -C "$repo" rev-parse HEAD)"
  echo "reference_commit=$reference_commit"
  echo "public_arepo_commit=$arepo_commit"
  echo "conda_env=$CONDA_DEFAULT_ENV"
  echo "systype=${SYSTYPE:-unset}"
  echo "cpus=$SLURM_CPUS_PER_TASK"
  printf "compiler="
  mpicxx --version | head -n 1
  printf "cxx="
  c++ --version | head -n 1
  module list 2>&1 || true
} >"$out/environment.txt"

git -C "$repo" status --porcelain=v1 >"$out/repo_status.txt"
git -C "$repo" ls-files -z | while IFS= read -r -d '' file; do
  sha256sum "$repo/$file"
done >"$out/tracked_source.sha256"
sha256sum "$repo/tests/frame2.png" "$repo/tests/frame2b.png" \
  >"$out/upstream_reference_images.sha256"

rsync -a --exclude='.git/' --exclude='arepo/' --exclude='build/' \
  --exclude='validation/' "$repo/" "$source_copy/"
rsync -a --exclude='.git/' "$dependency/" "$source_copy/arepo/"
mkdir -p "$source_copy/build"

unset PYTHONPATH
export PYTHONNOUSERSITE=1
make -C "$source_copy" -j"$SLURM_CPUS_PER_TASK" \
  >"$out/build.log" 2>&1
test -x "$source_copy/ArepoRT"
c++ -std=c++11 -Wall -Wextra -Werror -pedantic \
  "$source_copy/src/stellar_field_registry.cpp" \
  "$source_copy/tests/test_stellar_field_registry.cpp" \
  -o "$source_copy/build/test_stellar_field_registry" \
  >"$out/registry_build.log" 2>&1
"$source_copy/build/test_stellar_field_registry" \
  >"$out/registry_test.log" 2>&1
grep -q 'STELLAR_FIELD_REGISTRY_TEST_OK fields=9' "$out/registry_test.log"

run_render_suite() {
  local run_dir="$1"
  local log_dir="$2"
  local config output
  mkdir -p "$log_dir"
  for config in config_2.txt config_2b.txt config_3.txt; do
    output="$(awk '$1 == "imageFile" {print $3; exit}' "$source_copy/tests/$config")"
    rm -f "$source_copy/$output"
    (
      cd "$source_copy"
      ./ArepoRT "tests/$config"
    ) >"$log_dir/${config%.txt}.log" 2>&1
    test -s "$source_copy/$output"
    cp "$source_copy/$output" "$run_dir/$output"
  done
}

run_render_suite "$out/artifacts/run1" "$out/render_logs/run1"
run_render_suite "$out/artifacts/run2" "$out/render_logs/run2"

(
  cd "$out/artifacts/run1"
  sha256sum frame2.png frame2b.png frame3.png
) >"$out/render_run1.sha256"
(
  cd "$out/artifacts/run2"
  sha256sum frame2.png frame2b.png frame3.png
) >"$out/render_run2.sha256"
diff -u "$out/render_run1.sha256" "$out/render_run2.sha256" \
  >"$out/determinism.diff"

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
    if expected.shape != actual.shape:
        raise SystemExit(f"shape mismatch for {name}: {expected.shape} != {actual.shape}")
    delta = np.abs(expected.astype(np.int16) - actual.astype(np.int16))
    results[name] = {
        "shape": list(actual.shape),
        "exact": bool(np.array_equal(expected, actual)),
        "max_abs_channel_error": int(delta.max()),
        "rms_channel_error": float(np.sqrt(np.mean(delta.astype(np.float64) ** 2))),
        "different_values": int(np.count_nonzero(delta)),
    }
    if not results[name]["exact"]:
        raise SystemExit(f"pixel mismatch for {name}: {results[name]}")

with (out / "pixel_validation.json").open("w", encoding="ascii") as handle:
    json.dump(results, handle, indent=2, sort_keys=True)
    handle.write("\n")
PY

sha256sum "$source_copy/ArepoRT" "$source_copy/arepo/libarepo.a" \
  "$source_copy/build/test_stellar_field_registry" \
  >"$out/binaries.sha256"
sha256sum "$out"/artifacts/run1/*.png >"$out/reference_renders.sha256"

{
  echo "status=PASS"
  echo "completed=$(date -Is)"
  echo "upstream_render_tests=3"
  echo "repeat_render_tests=3"
  echo "pixel_reference_tests=2"
  echo "registry_unit_tests=1"
  echo "deterministic=true"
  echo "voronoi_reference_modified=false"
} >"$out/summary.txt"
touch "$out/PHASE0_VALIDATE_OK"
cat "$out/summary.txt"
echo "PHASE0_VALIDATE_OK output=$out host=$host job=$SLURM_JOB_ID"
