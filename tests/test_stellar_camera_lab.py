#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

import numpy as np


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import stellar_camera_spline as spline
import stellar_scene_camera_lab as lab


class StellarCameraLabTest(unittest.TestCase):
    def make_scene(self, path: Path, count: int = 2500) -> None:
        flags = lab.REQUIRED_FLAGS
        header = lab.HEADER_STRUCT.pack(
            lab.SCENE_MAGIC.ljust(16, b"\0"), 5, 0x01020304, 208, 52, 16,
            72, 16, 9, 16, 9, 8, flags, count, 0, 0, 0, 0,
            1.0e12, 1.0e12, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0,
            760.0, bytes(24))
        rng = np.random.default_rng(42)
        cells = np.zeros(count, dtype=lab.CELL_DTYPE)
        angle = rng.uniform(0.0, 2.0 * np.pi, count)
        radius = rng.lognormal(mean=np.log(7.0e10), sigma=0.35, size=count)
        cells["position"][:, 0] = 5.0e11 + radius * np.cos(angle)
        cells["position"][:, 1] = 5.0e11 + radius * np.sin(angle)
        cells["position"][:, 2] = 5.0e11 + rng.normal(0.0, 7.0e9, count)
        cells["density"] = (10.0 - np.log10(radius / 1.0e10)).astype(np.float32)
        cells["temperature"] = rng.lognormal(np.log(2.0e6), 0.7, count)
        cells["velocity"][:, 0] = (-np.sin(angle) * 1.5e8).astype(np.float32)
        cells["velocity"][:, 1] = (np.cos(angle) * 1.5e8).astype(np.float32)
        cells["velocity"][:, 2] = rng.normal(0.0, 2.0e7, count)
        cells["particle_id"] = np.arange(count, dtype=np.uint64) + 100
        with path.open("wb") as handle:
            handle.write(header)
            handle.write(cells.tobytes())

    def test_scene_parse_sampling_and_html(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            scene = root / "scene.bin"
            output = root / "viewer.html"
            self.make_scene(scene)
            header = lab.read_header(scene)
            self.assertEqual(header["num_cells"], 2500)
            cells = lab.read_cells(scene, header)
            center, axis = lab.infer_center_axis(cells, header, None, None)
            selected1 = lab.sample_cells(cells, 1000)
            selected2 = lab.sample_cells(cells, 1000)
            np.testing.assert_array_equal(selected1, selected2)
            self.assertEqual(selected1.size, 1000)
            payload = lab.build_payload(
                scene, header, cells, selected1, center, axis, None,
                lab.sha256(scene), 721, None)
            self.assertEqual(payload["point_count"], 1000)
            self.assertIn("rotational_fraction", payload["channels"])
            self.assertIn("outward_axial_velocity", payload["channels"])
            lab.write_html(output, payload)
            text = output.read_text(encoding="utf-8")
            self.assertIn("stellar_scene_camera_lab_v001", text)
            self.assertIn("Stellar Camera Lab", text)
            with self.assertRaises(FileExistsError):
                lab.write_html(output, payload)

    def test_spline_compiler(self) -> None:
        template = []
        for snapshot in range(10, 21):
            template.append([
                float(snapshot), float(snapshot - 9),
                0.0, 0.0, 4.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0,
                1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 3.0, 2.0, 4.0,
            ])
        keyframes = [
            {"snapshot": 10, "look_at_cm": [0.0, 0.0, 0.0],
             "view_direction": [0.0, 0.0, -1.0], "up": [0.0, 1.0, 0.0],
             "screen_half_extent_cm": 1.0},
            {"snapshot": 15, "look_at_cm": [0.2, 0.1, 0.0],
             "view_direction": [0.2, 0.0, -1.0], "up": [0.0, 1.0, 0.0],
             "screen_half_extent_cm": 1.3},
            {"snapshot": 20, "look_at_cm": [0.5, 0.4, 0.1],
             "view_direction": [0.35, 0.2, -1.0], "up": [0.0, 1.0, 0.2],
             "screen_half_extent_cm": 1.8},
        ]
        rows1, diagnostics1 = spline.compile_spline(template, keyframes, 0.25)
        rows2, diagnostics2 = spline.compile_spline(template, keyframes, 0.25)
        np.testing.assert_allclose(rows1, rows2, rtol=0.0, atol=0.0)
        self.assertEqual(diagnostics1, diagnostics2)
        self.assertEqual(len(rows1), 11)
        np.testing.assert_allclose(rows1[0][5:8], keyframes[0]["look_at_cm"],
                                   atol=1.0e-14)
        np.testing.assert_allclose(rows1[-1][5:8], keyframes[-1]["look_at_cm"],
                                   atol=1.0e-14)
        self.assertAlmostEqual(rows1[0][11], 1.0)
        self.assertAlmostEqual(rows1[-1][11], 1.8)
        for row in rows1:
            view = np.asarray(row[5:8]) - np.asarray(row[2:5])
            up = np.asarray(row[8:11])
            self.assertAlmostEqual(float(np.linalg.norm(up)), 1.0, places=12)
            self.assertAlmostEqual(float(np.dot(view / np.linalg.norm(view), up)),
                                   0.0, places=12)


if __name__ == "__main__":
    unittest.main()
