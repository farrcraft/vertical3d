# Draw curves

`Curves` draws thin ribbons along lines or smooth curves. Use it for hair, grass, wires and
lines. Both hiders draw curves.

## Draw a curve

```
Format 160 120 1
Projection "perspective" "fov" [40]
Clipping 0.1 100
WorldBegin
Surface "constant"
Color [0.9 0.9 0.3]
Curves "linear" [3] "nonperiodic" "P" [-1.5 -1 5  0 1 5  1.5 -1 5] "constantwidth" [0.05]
Color [0.3 0.8 0.9]
Basis "catmull-rom" 1 "catmull-rom" 1
Curves "cubic" [6] "nonperiodic"
    "P" [-1.5 0 5  -1.5 0 5  -0.5 0.8 5  0.5 -0.8 5  1.5 0 5  1.5 0 5]
    "width" [0.02 0.1 0.1 0.02]
WorldEnd
```

The yellow curve is a thin inverted V. The blue curve is a wave that is thick in the middle and
thin at each end.

The arguments of `Curves`:

1. `"linear"` joins the points with straight lines. `"cubic"` draws a smooth curve through
   them, shaped by the current `Basis`.
2. A list with the number of points in each curve. `[3]` is one curve of three points, and
   `[2 5]` is two curves, of two and five points.
3. `"nonperiodic"` for an open curve, or `"periodic"` for a closed one.
4. `"P"`, three numbers for each point, in the order of the curves.
5. The width: see [Width](#width).

Any other type or wrap is an error, and moya stops reading the file.

## Choose the basis of a cubic curve

`Basis` sets how a cubic curve's points shape it, for the curves that follow:

```
Basis "catmull-rom" 1 "catmull-rom" 1
```

Give the basis twice, once for each direction of a surface. A curve uses the second. Each name
is followed by its step, the number of points from one segment to the next, from 1 to 4.

| Basis | Usual step | The curve |
|---|---|---|
| `"bezier"` | 3 | Passes through every third point, and the points between pull it |
| `"b-spline"` | 1 | Passes near the points, smoothly |
| `"catmull-rom"` | 1 | Passes through every point but the first and the last |
| `"hermite"` | 2 | Each pair of values is a point and a tangent |
| `"power"` | 4 | The values are the coefficients of a polynomial |

A basis can also be 16 numbers, a 4 by 4 matrix. The basis is the one in force at the `Curves`
request. It is bezier with step 3 by default.

A nonperiodic cubic curve of n points has `(n - 4) / step + 1` segments. In the example, the
first and last points are repeated so that the catmull-rom curve reaches the ends.

## Width

- `"constantwidth" [w]` gives every curve in the request the width w.
- `"width"` gives a width at each end of each segment. A linear curve of n points has n widths.
  A nonperiodic cubic curve has one more width than it has segments.
- With neither, the width is 1.

The width is in object space, as the points are.

## What curves do not do

- A curve is a flat ribbon. It turns to face the camera under the reyes hider, and each ray
  under the ray-trace hider. It is never round, and `"N"` does not turn it.
- A varying `"Cs"` does not colour a curve. Use `Color` before the request.
- A curve does not join an area light.
- The ray-trace hider does not displace a curve.
- If the point counts reach past the end of `"P"`, moya draws what `"P"` holds and logs the
  rest.
