# Stellar GPU physical renderer v053f

`stellar_gpu_renderer_v053f` extends v053e without changing v073 scene or ray
geometry. It adds the retained v075 profile and `density_moment_v076`, including
post-ray scalar-moment copper-blue decoding and display brightness.

The manifest schema is:

```text
# schema=stellar_gpu_view_manifest_v053f
```

Each v053e row appends these values after `physical_reference_path_cm`:

```text
opacity_signal_threshold color_gamma color_invert
density_support_log10_low density_support_log10_high
emission_signal_floor display_brightness
```

All fields are present for every profile so rows have one deterministic shape.
For `density_moment_v076`, density high must exceed density low, gamma and
brightness must be positive, and emission floor must lie in `[0,1]`.

Build on an eta validation node with the workspace CUDA toolchain:

```bash
nvcc -std=c++14 -O3 -Xcompiler -fopenmp -Isrc \
  src/stellar_gpu_renderer_v053f.cu -o build/stellar_gpu_renderer_v053f
```

GPU runtime consumes only immutable verified v073 scenes. Raw snapshot reads and
scene export remain eta-only operations.
