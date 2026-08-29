#!/usr/bin/env python3
"""Audit the fixed 43-pose WebGL/Voronoi rotational-fraction acceptance set."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image


REQUIRED_COLUMNS = {
    "pose_id", "snapshot", "webgl_image", "voronoi_image",
    "webgl_camera_sha256", "voronoi_camera_sha256", "channel", "scale",
    "range_low", "range_high", "palette", "optical_profile",
}


def luma(path: Path) -> np.ndarray:
    image = np.asarray(Image.open(path).convert("RGB"), dtype=np.float64) / 255.0
    return image[..., 0] * 0.2126 + image[..., 1] * 0.7152 + image[..., 2] * 0.0722


def correlation(left: np.ndarray, right: np.ndarray) -> float:
    if left.shape != right.shape:
        right = np.asarray(Image.fromarray(
            np.asarray(right * 255.0, dtype=np.uint8)).resize(
                (left.shape[1], left.shape[0]), Image.Resampling.BILINEAR),
            dtype=np.float64) / 255.0
    a = left.ravel() - float(left.mean())
    b = right.ravel() - float(right.mean())
    denominator = float(np.linalg.norm(a) * np.linalg.norm(b))
    return float(np.dot(a, b) / denominator) if denominator > 0.0 else 0.0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ledger", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        raise FileExistsError(f"refusing to overwrite {args.output}")
    with args.ledger.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle, delimiter="\t"))
    if len(rows) != 43 or len({row["pose_id"] for row in rows}) != 43:
        raise ValueError("acceptance ledger must contain exactly 43 unique poses")
    missing = REQUIRED_COLUMNS - set(rows[0])
    if missing:
        raise ValueError(f"acceptance ledger is missing columns: {sorted(missing)}")

    metrics = []
    failures = []
    for row in rows:
        if row["webgl_camera_sha256"] != row["voronoi_camera_sha256"]:
            failures.append(f"{row['pose_id']}: camera provenance differs")
        if (row["channel"], row["scale"], row["range_low"],
                row["range_high"], row["palette"], row["optical_profile"]) != (
                "rotational_fraction", "linear", "0", "1", "copper_blue",
                "material_support_v074"):
            failures.append(f"{row['pose_id']}: transfer provenance differs")
        webgl = luma(Path(row["webgl_image"]))
        voronoi = luma(Path(row["voronoi_image"]))
        mean = float(voronoi.mean())
        standard_deviation = float(voronoi.std())
        bright_fraction = float(np.mean(voronoi > 0.9))
        coefficient_of_variation = standard_deviation / max(mean, 1.0e-8)
        image_correlation = correlation(voronoi, webgl)
        metric = {
            "pose_id": row["pose_id"], "snapshot": int(row["snapshot"]),
            "voronoi_luma_mean": mean, "voronoi_luma_std": standard_deviation,
            "voronoi_bright_fraction": bright_fraction,
            "voronoi_luma_cv": coefficient_of_variation,
            "webgl_voronoi_luma_correlation": image_correlation,
        }
        metrics.append(metric)
        if bright_fraction > 0.80:
            failures.append(f"{row['pose_id']}: white-sheet bright fraction {bright_fraction:.6f}")
        if coefficient_of_variation < 0.08:
            failures.append(f"{row['pose_id']}: insufficient normalized contrast {coefficient_of_variation:.6f}")
        if image_correlation < 0.10:
            failures.append(f"{row['pose_id']}: structure correlation {image_correlation:.6f}")

    median_luma = float(np.median([row["voronoi_luma_mean"] for row in metrics]))
    median_std = float(np.median([row["voronoi_luma_std"] for row in metrics]))
    median_correlation = float(np.median([
        row["webgl_voronoi_luma_correlation"] for row in metrics]))
    if not 0.02 <= median_luma <= 0.55:
        failures.append(f"median luma outside [0.02,0.55]: {median_luma:.6f}")
    if median_std < 0.045:
        failures.append(f"median luma std below 0.045: {median_std:.6f}")
    if median_correlation < 0.35:
        failures.append(f"median structure correlation below 0.35: {median_correlation:.6f}")
    payload = {
        "schema": "stellar_physical_visual_acceptance_v074",
        "status": "PASS" if not failures else "FAIL",
        "pose_count": len(metrics),
        "requirements": {
            "maximum_per_pose_bright_fraction": 0.80,
            "minimum_per_pose_luma_cv": 0.08,
            "minimum_per_pose_structure_correlation": 0.10,
            "median_luma_interval": [0.02, 0.55],
            "minimum_median_luma_std": 0.045,
            "minimum_median_structure_correlation": 0.35,
        },
        "aggregate": {
            "median_luma": median_luma, "median_luma_std": median_std,
            "median_structure_correlation": median_correlation,
        },
        "failures": failures, "poses": metrics,
        "ledger_sha256": hashlib.sha256(args.ledger.read_bytes()).hexdigest(),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, allow_nan=False)
        handle.write("\n")
    print("STELLAR_PHYSICAL_VISUAL_V074_" + payload["status"])
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
