#!/usr/bin/env python3
"""Build no-clobber v053h composite-support acceptance manifests."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
from dataclasses import asdict, dataclass
from pathlib import Path


EXPECTED_INPUT_SCHEMA = "# schema=stellar_gpu_view_manifest_v053d"
OUTPUT_SCHEMA = "# schema=stellar_gpu_view_manifest_v053h"
EXPECTED_POSES = 43
EXPECTED_INPUT_COLUMNS = 41
ROTATIONAL_CHANNEL = "rotational_fraction"
POSE_PREFIX = re.compile(r"^pose_(\d{4})__rotational_fraction$")


@dataclass(frozen=True)
class Wave:
    name: str
    target_optical_depth: float
    target_emission: float
    density_log10_low: float
    density_log10_high: float
    emission_signal_floor: float

    @classmethod
    def parse(cls, text: str) -> "Wave":
        fields = text.split(":")
        if len(fields) != 6:
            raise argparse.ArgumentTypeError(
                "wave must be NAME:TAU:EMISSION:DENSITY_LOW:DENSITY_HIGH:FLOOR"
            )
        try:
            wave = cls(fields[0], *(float(value) for value in fields[1:]))
        except ValueError as error:
            raise argparse.ArgumentTypeError(str(error)) from error
        if not re.fullmatch(r"[a-z][a-z0-9_]*", wave.name):
            raise argparse.ArgumentTypeError(f"invalid wave name: {wave.name}")
        if not wave.target_optical_depth > 0 or not wave.target_emission > 0:
            raise argparse.ArgumentTypeError("wave tau and emission must be positive")
        if not wave.density_log10_high > wave.density_log10_low:
            raise argparse.ArgumentTypeError("wave density high must exceed low")
        if not 0 <= wave.emission_signal_floor <= 1:
            raise argparse.ArgumentTypeError("wave emission floor must be in [0,1]")
        return wave


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def reviewed_style(bundle: dict, preset_name: str) -> dict:
    presets = bundle.get("style_presets")
    if not isinstance(presets, list):
        raise ValueError("review bundle has no style_presets list")
    matches = [preset for preset in presets if preset.get("name") == preset_name]
    if len(matches) != 1:
        raise ValueError(f"expected one style preset named {preset_name!r}")
    style = matches[0].get("visual_state")
    if not isinstance(style, dict):
        raise ValueError("reviewed preset lacks visual_state")
    required = {
        "channel",
        "scale_mode",
        "low",
        "high",
        "symlog_threshold",
        "palette",
        "inversion",
        "gamma",
        "saturation",
        "brightness",
    }
    missing = required.difference(style)
    if missing:
        raise ValueError(f"reviewed preset missing: {sorted(missing)}")
    if style["channel"] != ROTATIONAL_CHANNEL:
        raise ValueError("reviewed preset is not rotational_fraction")
    if style["palette"] != "copper_blue":
        raise ValueError("reviewed preset is not copper_blue")
    if style["scale_mode"] != "linear":
        raise ValueError("reviewed preset is not linear")
    if not float(style["high"]) > float(style["low"]):
        raise ValueError("reviewed preset range is invalid")
    if not float(style["gamma"]) > 0 or not float(style["brightness"]) > 0:
        raise ValueError("reviewed gamma and brightness must be positive")
    return style


def load_pose_contract(bundle: dict) -> list[dict]:
    geometry = bundle.get("geometry")
    alternatives = geometry.get("alternatives") if isinstance(geometry, dict) else None
    if not isinstance(alternatives, list) or len(alternatives) != EXPECTED_POSES:
        raise ValueError(f"review bundle must contain exactly {EXPECTED_POSES} alternatives")
    for index, pose in enumerate(alternatives):
        if not pose.get("pose_id") or not isinstance(pose.get("snapshot"), int):
            raise ValueError(f"pose {index} lacks pose_id or integer snapshot")
    return alternatives


def load_rotational_rows(input_root: Path, poses: list[dict]) -> dict[int, list[list[str]]]:
    manifests = sorted(input_root.glob("snapshot_*/view_manifest_v053d.tsv"))
    if not manifests:
        raise ValueError(f"no v053d manifests under {input_root}")
    rows_by_snapshot: dict[int, list[list[str]]] = {}
    seen_pose_indices: set[int] = set()
    for manifest in manifests:
        lines = manifest.read_text(encoding="utf-8").splitlines()
        if not lines or lines[0] != EXPECTED_INPUT_SCHEMA:
            raise ValueError(f"unexpected schema in {manifest}")
        selected: list[list[str]] = []
        for line in lines[1:]:
            if not line or line.startswith("#"):
                continue
            fields = shlex.split(line)
            if len(fields) != EXPECTED_INPUT_COLUMNS:
                raise ValueError(f"expected 41 columns in {manifest}, got {len(fields)}")
            if fields[34] != ROTATIONAL_CHANNEL:
                continue
            match = POSE_PREFIX.fullmatch(fields[3])
            if not match:
                raise ValueError(f"unexpected rotational output prefix: {fields[3]}")
            pose_index = int(match.group(1))
            if pose_index in seen_pose_indices or pose_index >= len(poses):
                raise ValueError(f"duplicate or out-of-range pose index {pose_index}")
            snapshot = int(manifest.parent.name.removeprefix("snapshot_"))
            if poses[pose_index]["snapshot"] != snapshot:
                raise ValueError(
                    f"pose {pose_index} snapshot mismatch: "
                    f"{poses[pose_index]['snapshot']} != {snapshot}"
                )
            selected.append(fields)
            seen_pose_indices.add(pose_index)
        if selected:
            rows_by_snapshot[snapshot] = selected
    if seen_pose_indices != set(range(EXPECTED_POSES)):
        missing = sorted(set(range(EXPECTED_POSES)).difference(seen_pose_indices))
        raise ValueError(f"rotational manifests do not cover all poses; missing {missing}")
    return rows_by_snapshot


def format_float(value: float) -> str:
    return format(float(value), ".17g")


def build_manifest_row(
    fields: list[str],
    local_index: int,
    wave: Wave,
    style: dict,
    reference_path_cm: float,
) -> list[str]:
    output = list(fields)
    output[0] = str(local_index)
    output[3] = f"{wave.name}__{fields[3]}"
    output[33] = format_float(style["saturation"])
    output[36] = format_float(style["low"])
    output[37] = format_float(style["high"])
    output[38] = format_float(style["symlog_threshold"])
    output.extend(
        [
            "composite_moment_v078",
            format_float(wave.target_optical_depth),
            format_float(wave.target_emission),
            format_float(reference_path_cm),
            "0.02",
            format_float(style["gamma"]),
            "1" if style["inversion"] else "0",
            format_float(wave.density_log10_low),
            format_float(wave.density_log10_high),
            format_float(wave.emission_signal_floor),
            format_float(style["brightness"]),
        ]
    )
    if len(output) != 52:
        raise AssertionError(f"v053h row has {len(output)} columns, expected 52")
    return output


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-root", type=Path, required=True)
    parser.add_argument("--review-bundle", type=Path, required=True)
    parser.add_argument("--review-bundle-sha256", required=True)
    parser.add_argument("--preset-name", default="copper_blue_high_gamma")
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--reference-path-cm", type=float, default=0.0)
    parser.add_argument("--wave", action="append", type=Wave.parse, required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.output_root.exists():
        raise SystemExit(f"refusing to overwrite existing output root: {args.output_root}")
    if args.reference_path_cm < 0:
        raise SystemExit("reference path must be zero (automatic) or positive")
    if sha256(args.review_bundle) != args.review_bundle_sha256:
        raise SystemExit("review bundle SHA-256 mismatch")
    if len({wave.name for wave in args.wave}) != len(args.wave):
        raise SystemExit("wave names must be unique")

    bundle = json.loads(args.review_bundle.read_text(encoding="utf-8"))
    style = reviewed_style(bundle, args.preset_name)
    poses = load_pose_contract(bundle)
    rows_by_snapshot = load_rotational_rows(args.input_root, poses)

    args.output_root.mkdir(parents=True)
    manifests: list[dict] = []
    for wave in args.wave:
        for snapshot, rows in sorted(rows_by_snapshot.items()):
            destination = args.output_root / wave.name / f"snapshot_{snapshot:04d}"
            destination.mkdir(parents=True)
            manifest = destination / "view_manifest_v053h.tsv"
            output_lines = [OUTPUT_SCHEMA]
            for local_index, fields in enumerate(rows):
                output_lines.append(
                    " ".join(
                        build_manifest_row(
                            fields, local_index, wave, style, args.reference_path_cm
                        )
                    )
                )
            manifest.write_text("\n".join(output_lines) + "\n", encoding="utf-8")
            manifests.append(
                {
                    "wave": wave.name,
                    "snapshot": snapshot,
                    "pose_count": len(rows),
                    "path": str(manifest),
                    "sha256": sha256(manifest),
                }
            )

    package = {
        "schema": "stellar_gpu_v053h_acceptance_package_v001",
        "source_manifest_schema": EXPECTED_INPUT_SCHEMA.removeprefix("# schema="),
        "output_manifest_schema": OUTPUT_SCHEMA.removeprefix("# schema="),
        "review_bundle": str(args.review_bundle),
        "review_bundle_sha256": args.review_bundle_sha256,
        "source_pose_bundle_sha256": bundle.get("source_pose_bundle", {}).get("sha256"),
        "preset_name": args.preset_name,
        "style": {
            key: style[key]
            for key in (
                "channel",
                "scale_mode",
                "low",
                "high",
                "symlog_threshold",
                "palette",
                "inversion",
                "gamma",
                "saturation",
                "brightness",
            )
        },
        "reference_path_cm": args.reference_path_cm,
        "waves": [asdict(wave) for wave in args.wave],
        "pose_count": len(poses),
        "poses": [
            {
                "pose_index": index,
                "pose_id": pose["pose_id"],
                "snapshot": pose["snapshot"],
                "scene_sha256": pose.get("scene_sha256"),
            }
            for index, pose in enumerate(poses)
        ],
        "manifest_count": len(manifests),
        "render_count": len(poses) * len(args.wave),
        "manifests": manifests,
        "raw_snapshot_reads": 0,
        "scene_exports": 0,
    }
    package_path = args.output_root / "acceptance_package.json"
    package_path.write_text(
        json.dumps(package, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        "STELLAR_GPU_V053H_ACCEPTANCE_PACKAGE_OK "
        f"poses={len(poses)} waves={len(args.wave)} renders={package['render_count']} "
        f"manifests={len(manifests)} package_sha256={sha256(package_path)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
