# Stellar feature landmarks v066

## Purpose

The v066 sidecar separates physical feature support from image-edge contact.
The v064 report retains raw selected-cell minima and maxima, which are useful
for proving clipping but are unstable camera targets when a small amount of
diffuse material lies far from the dominant structure. The v066 report adds
weighted screen-space and physical quantiles without changing the v064 schema
or any transfer function.

## Contract

`stellar_feature_probe_v064` accepts an optional `--landmark-output PATH`.
When supplied, it writes `stellar_feature_landmarks_v066` alongside the
unchanged v064 report. The sidecar records, for every feature and weight
threshold:

- weighted screen center relative to the current frame center;
- weighted absolute screen support at q90, q95, and q99 on each axis;
- weighted screen half-extent at q90, q95, and q99;
- cylindrical-radius quantiles normalized by the physical disk radius;
- absolute-height quantiles and signed-height center normalized by the
  physical polar outer extent;
- the raw maximum projected support and edge-censoring flag from v064.

Screen quantiles use all selected cell centers, including centers outside the
current image. They are therefore not clipped to the viewport. A low-weight
far outlier cannot dominate q99 unless it contributes at least one percent of
the selected feature weight.

## Camera use

The sidecar is diagnostic input, not an automatic camera command. A candidate
orthographic correction for a chosen feature and threshold may be estimated
from `weighted_screen_half_extent_q99 / target_half_extent_fraction`, while the
weighted screen center provides a separate recentering observation. The raw
censoring columns remain mandatory review evidence.

Disk and positive/negative polar rows must remain separate. The camera
director must bind any adopted quantile, threshold, profile, scene hash, and
source camera path in a versioned artistic shot specification. It must not
silently replace physical tracking or crop the opposite lobe.

## Release policy

- The v064 report and all renderer equations remain unchanged.
- `legacy_v064` remains the default feature profile.
- GPU rendering is unaffected; v066 is a CPU-side packed-scene diagnostic.
- Synthetic tests prove weighted-quantile outlier resistance, monotonic
  q90/q95/q99 values, deterministic output, strict input, and no-clobber.
- Real-scene acceptance must reuse immutable eta-produced packed scenes before
  any camera or rendering campaign consumes v066 landmarks.
