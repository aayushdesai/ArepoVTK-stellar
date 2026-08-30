#!/usr/bin/env python3
"""Build no-clobber v053k adaptive-projection manifests."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import shlex
from pathlib import Path


EXPECTED_INPUT_SCHEMA = "# schema=stellar_gpu_view_manifest_v053d"
OUTPUT_SCHEMA = "# schema=stellar_gpu_view_manifest_v053k"
EXPECTED_POSES = 43
EXPECTED_INPUT_COLUMNS = 41
OUTPUT_COLUMNS = 58
POSE_PREFIX = re.compile(r"^pose_(\d{4})__rotational_fraction$")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def format_float(value: float) -> str:
    return format(float(value), ".17g")


def reviewed_styles(bundle: dict, preset_name: str, poses: list[dict]) -> dict[int, dict]:
    presets = bundle.get("style_presets")
    matches = [preset for preset in presets or [] if preset.get("name") == preset_name]
    if len(matches) != 1:
        raise ValueError(f"expected one style preset named {preset_name!r}")
    style = matches[0].get("visual_state")
    required = {
        "channel", "scale_mode", "low", "high", "symlog_threshold",
        "palette", "inversion", "gamma", "saturation", "brightness",
    }
    if not isinstance(style, dict) or required.difference(style):
        raise ValueError("reviewed preset lacks the required visual state")
    if style["channel"] != "rotational_fraction":
        raise ValueError("hero styling must be bound to rotational_fraction")
    if style["scale_mode"] != "linear" or style["palette"] != "copper_blue":
        raise ValueError("hero styling requires linear copper_blue")
    low = float(style["low"])
    high = float(style["high"])
    if not 0.0 <= low < high <= 1.0:
        raise ValueError("hero rotational range must lie within [0,1]")
    if not float(style["gamma"]) > 0 or not float(style["brightness"]) > 0:
        raise ValueError("hero gamma and brightness must be positive")
    styles = {index: dict(style) for index in range(len(poses))}
    pose_index = {pose["pose_id"]: index for index, pose in enumerate(poses)}
    latest: dict[str, dict] = {}
    for binding in bundle.get("pose_style_bindings") or []:
        pose_id = binding.get("pose_id")
        visual = binding.get("visual_state")
        if pose_id not in pose_index or not isinstance(visual, dict):
            continue
        if visual.get("channel") != "rotational_fraction":
            continue
        previous = latest.get(pose_id)
        if previous is None or str(binding.get("created_at", "")) > str(
            previous.get("created_at", "")
        ):
            latest[pose_id] = binding
    for pose_id, binding in latest.items():
        visual = binding["visual_state"]
        if required.difference(visual):
            raise ValueError(f"pose override {pose_id} lacks required visual state")
        if visual["scale_mode"] != "linear" or visual["palette"] != "copper_blue":
            raise ValueError(f"pose override {pose_id} is not linear copper_blue")
        low = float(visual["low"])
        high = float(visual["high"])
        if not 0.0 <= low < high <= 1.0:
            raise ValueError(f"pose override {pose_id} has an invalid range")
        if not float(visual["gamma"]) > 0 or not float(visual["brightness"]) > 0:
            raise ValueError(f"pose override {pose_id} has an invalid display gain")
        if visual.get("scene_sha256") != poses[pose_index[pose_id]]["scene_sha256"]:
            raise ValueError(f"pose override {pose_id} scene hash mismatch")
        styles[pose_index[pose_id]] = dict(visual)
    return styles


def load_poses(bundle: dict) -> list[dict]:
    geometry = bundle.get("geometry")
    alternatives = geometry.get("alternatives") if isinstance(geometry, dict) else None
    if not isinstance(alternatives, list) or len(alternatives) != EXPECTED_POSES:
        raise ValueError(f"review bundle must contain {EXPECTED_POSES} alternatives")
    return alternatives


def load_rows(input_root: Path, poses: list[dict]) -> dict[int, list[list[str]]]:
    rows_by_snapshot: dict[int, list[list[str]]] = {}
    seen: set[int] = set()
    for manifest in sorted(input_root.glob("snapshot_*/view_manifest_v053d.tsv")):
        lines = manifest.read_text(encoding="utf-8").splitlines()
        if not lines or lines[0] != EXPECTED_INPUT_SCHEMA:
            raise ValueError(f"unexpected schema in {manifest}")
        selected: list[list[str]] = []
        for line in lines[1:]:
            if not line or line.startswith("#"):
                continue
            fields = shlex.split(line)
            if len(fields) != EXPECTED_INPUT_COLUMNS:
                raise ValueError(f"expected 41 columns in {manifest}")
            if fields[34] != "rotational_fraction":
                continue
            match = POSE_PREFIX.fullmatch(fields[3])
            if not match:
                raise ValueError(f"unexpected pose prefix {fields[3]}")
            pose_index = int(match.group(1))
            snapshot = int(manifest.parent.name.removeprefix("snapshot_"))
            if pose_index in seen or poses[pose_index]["snapshot"] != snapshot:
                raise ValueError(f"pose {pose_index} is duplicated or misbound")
            selected.append(fields)
            seen.add(pose_index)
        if selected:
            rows_by_snapshot[snapshot] = selected
    if seen != set(range(EXPECTED_POSES)):
        raise ValueError("input manifests do not cover all reviewed alternatives")
    return rows_by_snapshot


def build_row(
    fields: list[str], index: int, style: dict, contract: dict,
    contract_sha256: str,
) -> list[str]:
    output = list(fields)
    output[0] = str(index)
    output[3] = f"projection_v081__{fields[3]}"
    output[33] = format_float(style["saturation"])
    output[35] = "linear"
    output[36] = format_float(style["low"])
    output[37] = format_float(style["high"])
    output[38] = format_float(style["symlog_threshold"])
    output.extend(
        [
            "legacy_v072", "1", "1", "0", "0.02",
            format_float(style["gamma"]),
            "1" if style["inversion"] else "0",
            "-9", "-5", "0.25", format_float(style["brightness"]),
            "hero_adaptive_copper_blue_v081",
            format_float(contract["reference_quantile"]),
            format_float(contract["response_floor_fraction"]),
            format_float(style["opacity"]),
            format_float(style["opacity"]),
            contract_sha256,
        ]
    )
    if len(output) != OUTPUT_COLUMNS:
        raise AssertionError(f"v053k row has {len(output)} columns")
    return output


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-root", type=Path, required=True)
    parser.add_argument("--review-bundle", type=Path, required=True)
    parser.add_argument("--review-bundle-sha256", required=True)
    parser.add_argument("--preset-name", default="copper_blue_high_gamma")
    parser.add_argument("--pose-index", type=int, action="append")
    parser.add_argument("--output-root", type=Path, required=True)
    args = parser.parse_args()

    if args.output_root.exists():
        raise SystemExit(f"refusing to overwrite {args.output_root}")
    if sha256(args.review_bundle) != args.review_bundle_sha256:
        raise SystemExit("review bundle SHA-256 mismatch")
    bundle = json.loads(args.review_bundle.read_text(encoding="utf-8"))
    poses = load_poses(bundle)
    styles = reviewed_styles(bundle, args.preset_name, poses)
    for pose_index, style in styles.items():
        opacity = float(style.get("opacity", 0.0))
        if not 0.0 < opacity <= 1.0:
            raise SystemExit(f"pose {pose_index} has opacity outside (0,1]")
    rows_by_snapshot = load_rows(args.input_root, poses)
    selected_indices = set(args.pose_index or range(EXPECTED_POSES))
    if not selected_indices or min(selected_indices) < 0 or max(selected_indices) >= EXPECTED_POSES:
        raise SystemExit("selected pose index is out of range")

    args.output_root.mkdir(parents=True)
    contract = {
        "schema": "stellar_hero_display_contract_v081",
        "review_bundle_sha256": args.review_bundle_sha256,
        "reference_scope": "per_view_positive_ray_quantile",
        "reference_quantile": 0.99,
        "response_floor_fraction": 0.02,
        "display_encoding": "camera_lab_direct_rgb",
        "palette": "copper_blue",
        "gain_source": "reviewed_pose_opacity",
        "projection": "per_ray_dominant_cell",
        "physical_samples": [
            "material_peak_density", "material_rotation",
            "material_hot_support", "outflow_peak_flux",
        ],
        "static_structure_weights": False,
        "false_color_integrated_as_radiance": False,
    }
    contract_path = args.output_root / "hero_display_contract_v081.json"
    contract_path.write_text(
        json.dumps(contract, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    contract_sha256 = sha256(contract_path)
    manifests: list[dict] = []
    selected_pose_count = 0
    for snapshot, rows in sorted(rows_by_snapshot.items()):
        rows = [
            fields for fields in rows
            if int(POSE_PREFIX.fullmatch(fields[3]).group(1)) in selected_indices
        ]
        if not rows:
            continue
        destination = args.output_root / f"snapshot_{snapshot:04d}"
        destination.mkdir(parents=True)
        manifest = destination / "view_manifest_v053k.tsv"
        lines = [OUTPUT_SCHEMA]
        for local_index, fields in enumerate(rows):
            pose_index = int(POSE_PREFIX.fullmatch(fields[3]).group(1))
            lines.append(
                " ".join(
                    build_row(
                        fields, local_index, styles[pose_index], contract,
                        contract_sha256,
                    )
                )
            )
        manifest.write_text("\n".join(lines) + "\n", encoding="utf-8")
        manifests.append(
            {
                "snapshot": snapshot,
                "pose_count": len(rows),
                "path": str(manifest),
                "sha256": sha256(manifest),
            }
        )
        selected_pose_count += len(rows)

    package = {
        "schema": "stellar_gpu_v053k_projection_package_v001",
        "review_bundle": str(args.review_bundle),
        "review_bundle_sha256": args.review_bundle_sha256,
        "display_contract": str(contract_path),
        "display_contract_sha256": contract_sha256,
        "render_program": "hero_adaptive_copper_blue_v081",
        "projection": contract["projection"],
        "components": [
            "material_peak_density", "material_rotation",
            "material_hot_support", "outflow_peak_flux",
        ],
        "static_structure_weights": False,
        "false_color_integrated_as_radiance": False,
        "reference_scope": contract["reference_scope"],
        "reference_quantile": contract["reference_quantile"],
        "response_floor_fraction": contract["response_floor_fraction"],
        "gain_source": contract["gain_source"],
        "reviewed_pose_count": len(poses),
        "pose_count": selected_pose_count,
        "selected_pose_indices": sorted(selected_indices),
        "render_count": selected_pose_count,
        "manifests": manifests,
        "raw_snapshot_reads": 0,
        "scene_exports": 0,
    }
    package_path = args.output_root / "projection_package.json"
    package_path.write_text(
        json.dumps(package, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        "STELLAR_GPU_V053K_PROJECTION_PACKAGE_OK "
        f"poses={selected_pose_count} manifests={len(manifests)} "
        f"package_sha256={sha256(package_path)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
