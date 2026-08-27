# Stellar feature framing v067

## Purpose

The v067 plan converts selected-cell screen coordinates into a reproducible
framing recommendation without editing a camera path. It complements the v066
absolute quantiles with signed bounds so an off-center disk or asymmetric
bipolar outflow can be recentered and scaled in one calculation.

## Inputs

The probe accepts a complete, named framing request only when
`--framing-output` is supplied:

- mode `single` with one named feature, or `bipolar_union` with exactly two;
- a feature-weight threshold already present in the probe grid;
- central coverage `0.90` (q05 to q95) or `0.98` (q01 to q99);
- independent target half-width and half-height fractions in `(0, 1]`.

Incomplete requests, duplicate features, absent selected features, invalid
bounds, and existing or identical output paths are rejected before any output
is written.

## Recommendation

For each requested feature, v067 records signed lower and upper screen bounds.
The union is the minimum lower and maximum upper bound over all members. The
recommended look-at shift is the midpoint of that union in the source camera's
normalized screen basis. A positive shift moves the frame center toward the
positive screen direction. The scale factor is

`max(robust_half_width / target_half_width,
     robust_half_height / target_half_height)`.

The proposed orthographic half extent is the source half extent multiplied by
that factor. The plan does not write Cartesian coordinates, camera paths, or
shots. Operations must bind the source scene, camera-path row, config, and
their checksums before using the recommendation.

## Scientific boundaries

- Positive and negative polar lobes remain separate member rows; the union
  cannot crop the broader lobe silently.
- Raw right-censoring is retained for every member but is not used as a radius
  or divided into a robust quantile.
- The v064 report and v066 sidecar remain byte-for-byte unchanged.
- Selector, palette, renderer, and native C++ Voronoi equations are unchanged.
- A plan is diagnostic until a bounded contact pilot validates its target and
  transition behavior.
