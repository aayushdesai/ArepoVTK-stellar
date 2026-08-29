# Stellar GPU physical renderer v053g

`stellar_gpu_renderer_v053g` extends v053f with
`normalized_moment_v077`. It consumes the existing immutable v073 mesh and
ray payloads and retains every older optical profile.

The v053g view-manifest row has the same 52 columns as v053f. The optical
profile field may additionally be `normalized_moment_v077`; density support,
target optical depth/emission, color gamma, inversion, and display brightness
remain explicit manifest inputs.

The focused acceptance builder is:

```text
tools/build_stellar_gpu_v053g_acceptance.py
```

For the current correction, generate one 43-pose rotational-fraction wave
using the reviewed copper-blue preset. Do not launch a movie until the matched
WebGL/native still contacts pass the contrast, chroma, and non-white-sheet
review gates.
