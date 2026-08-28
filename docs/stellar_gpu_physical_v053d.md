# Stellar GPU physical renderer v053d

`stellar_gpu_renderer_v053d` extends the accepted v053c CUDA Voronoi marcher
with the exact native v071/v072 physical-channel evaluators. It supports all
24 non-optical channels from `density` through `mach_number` and uses the
native copper-blue physical transfer. Physical rows require `voronoi`
reconstruction.

The renderer consumes packed scene schema v073. Set this environment variable
when exporting through `ArepoRT`:

```text
AREPORT_STELLAR_SCENE_FORMAT=v073
```

The default remains v052, so existing exporters, scenes, and v053c are
unchanged. V073 adds magnetic field in gauss, gas pressure in dyn/cm2, and
sound speed in cm/s to every cell. Density, temperature, velocity, particle
IDs, topology, and rays retain their prior canonical representations.

The view manifest schema line is:

```text
# schema=stellar_gpu_view_manifest_v053d
```

Each v053c row appends these columns:

```text
physical_channel physical_scale range_min range_max symlog_linthresh physical_opacity physical_emission
```

Ranges are in transformed units, matching native v071/v072. Signed channels
may use `symlog`; positive channels may use `linear` or `log10`. The renderer
records the selected channel, scale, range, opacity, emission, and copper-blue
palette in every benchmark report.

Build commands:

```bash
nvcc -std=c++14 -O3 -Xcompiler=-fopenmp -Isrc \
  src/stellar_gpu_renderer_v053d.cu -o build/stellar_gpu_renderer_v053d
c++ -std=c++11 -O2 -Isrc tests/test_stellar_gpu_physical_contract_v053d.cpp \
  -o build/test_stellar_gpu_physical_contract_v053d
```

For movie generation, export each simulation snapshot once as a full v073
scene at the selected camera row, then render all channel manifests from that
immutable scene. Ray-only payloads remain available for multiple camera views
of one resident mesh.
