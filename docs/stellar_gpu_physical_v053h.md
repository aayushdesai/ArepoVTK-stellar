# Stellar GPU physical renderer v053h

`stellar_gpu_renderer_v053h` adds the v078 composite-support profile to the
retained v053g renderer contract. It consumes the same immutable v073 scene and
ray payloads and the same 52-column view-manifest layout.

The only new accepted profile is `composite_moment_v078`. The GPU renderer uses
the same physical sample, v065 feature weights, optical evaluator, normalized
decoder, filmic display transform, and copper-blue byte encoding as the native
path. Existing v072-v077 profiles remain accepted without behavioral changes.

The acceptance-package generator is
`tools/build_stellar_gpu_v053h_acceptance.py`. It emits no-clobber manifests for
the exact reviewed 43-pose bundle and performs no snapshot reads or scene
exports.
