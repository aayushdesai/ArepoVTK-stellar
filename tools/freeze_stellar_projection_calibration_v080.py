#!/usr/bin/env python3
"""Freeze robust v080 projection references from representative ray reports."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import statistics
from pathlib import Path


SCHEMA = "stellar_hero_projection_calibration_v080"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_report(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        values[key] = value
    required = {
        "renderer_version",
        "render_program",
        "projection_material_column_p90_g_cm2",
        "projection_outflow_column_flux_p95_g_cm_s",
        "ray_limit_mode",
        "base_scene",
        "ray_scene",
        "view_valid",
    }
    missing = required.difference(values)
    if missing:
        raise ValueError(f"{path} missing report keys: {sorted(missing)}")
    if values["renderer_version"] != "stellar_v053j":
        raise ValueError(f"{path} is not a v053j report")
    if values["render_program"] != "hero_projected_copper_blue_v080":
        raise ValueError(f"{path} is not a v080 projection report")
    if values["ray_limit_mode"] == "legacy_absolute":
        raise ValueError(f"{path} uses a legacy absolute ray limit")
    if values["view_valid"] != "true":
        raise ValueError(f"{path} did not pass renderer validity")
    return values


def parse_epoch(text: str) -> tuple[int, Path]:
    snapshot, separator, filename = text.partition(":")
    if not separator:
        raise argparse.ArgumentTypeError("epoch must be SNAPSHOT:BENCHMARK")
    try:
        snapshot_index = int(snapshot)
    except ValueError as error:
        raise argparse.ArgumentTypeError(str(error)) from error
    if snapshot_index < 0:
        raise argparse.ArgumentTypeError("snapshot must be nonnegative")
    return snapshot_index, Path(filename)


def geometric_median(values: list[float]) -> float:
    if not values or any(not math.isfinite(value) or value <= 0 for value in values):
        raise ValueError("calibration quantiles must be finite and positive")
    return math.exp(statistics.median(math.log(value) for value in values))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--epoch", action="append", type=parse_epoch, required=True)
    parser.add_argument("--source-manifest", type=Path, required=True)
    parser.add_argument("--review-bundle", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.output.exists():
        raise SystemExit(f"refusing to overwrite {args.output}")
    if len(args.epoch) < 3:
        raise SystemExit("at least three representative epochs are required")
    snapshots = [snapshot for snapshot, _ in args.epoch]
    if len(set(snapshots)) != len(snapshots):
        raise SystemExit("representative snapshots must be unique")

    rows: list[dict] = []
    material: list[float] = []
    outflow: list[float] = []
    for snapshot, path in sorted(args.epoch):
        report = parse_report(path)
        material_value = float(report["projection_material_column_p90_g_cm2"])
        outflow_value = float(
            report["projection_outflow_column_flux_p95_g_cm_s"]
        )
        if not math.isfinite(material_value) or material_value < 0:
            raise SystemExit(f"snapshot {snapshot} has invalid material p90")
        if not math.isfinite(outflow_value) or outflow_value < 0:
            raise SystemExit(f"snapshot {snapshot} has invalid outflow p95")
        if material_value > 0:
            material.append(material_value)
        if outflow_value > 0:
            outflow.append(outflow_value)
        rows.append(
            {
                "snapshot": snapshot,
                "benchmark": str(path),
                "benchmark_sha256": sha256(path),
                "base_scene": report["base_scene"],
                "ray_scene": report["ray_scene"],
                "material_column_p90_g_cm2": material_value,
                "outflow_column_flux_p95_g_cm_s": outflow_value,
                "material_signal_present": material_value > 0,
                "outflow_signal_present": outflow_value > 0,
            }
        )

    if len(material) < 2:
        raise SystemExit("at least two representative epochs need material signal")
    if len(outflow) < 2:
        raise SystemExit("at least two representative epochs need outflow signal")

    product = {
        "schema": SCHEMA,
        "method": {
            "material": "geometric_median_of_epoch_positive_ray_p90",
            "outflow": "geometric_median_of_epoch_positive_ray_p95",
            "outflow_local_quantity": "rho_times_positive_axial_velocity_times_coherence_squared",
            "material_support": "reviewed_rotational_range_union_hot_core",
            "static_structure_weights": False,
            "false_color_integrated_as_radiance": False,
        },
        "source_manifest": str(args.source_manifest),
        "source_manifest_sha256": sha256(args.source_manifest),
        "review_bundle": str(args.review_bundle),
        "review_bundle_sha256": sha256(args.review_bundle),
        "epochs": rows,
        "material_column_reference_g_cm2": geometric_median(material),
        "outflow_column_flux_reference_g_cm_s": geometric_median(outflow),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(
        json.dumps(product, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        "STELLAR_HERO_PROJECTION_CALIBRATION_V080_OK "
        f"epochs={len(rows)} output_sha256={sha256(args.output)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
