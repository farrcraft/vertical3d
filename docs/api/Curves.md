# Curves

This page is for someone writing against the api. It covers `api/curve`: the four types of
curve, how each is evaluated, how a Bezier curve's handles behave, the geometry that holds many
curves, and the conversion to the basis curves a renderer takes. Terms are defined in the
[glossary](README.md#glossary).

- [What the library is](#what-the-library-is)
- [The four types](#the-four-types)
- [Bezier handles](#bezier-handles)
- [Evaluation](#evaluation)
- [The geometry](#the-geometry)
- [Basis curves](#basis-curves)

## What the library is

`v3dlib_curve`, namespace `v3d::curve`, depends on glm only. It names no device, window or
renderer, so it runs and is tested anywhere. The editor's curve items hold its geometry, and an
importer or a modelling tool can build curves with it without a scene.

A `Curve` ([api/curve/Curve.h](../../api/curve/Curve.h)) is one curve of any type: a
`std::variant` of `Poly`, `Bezier`, `CatmullRom` and `Nurbs`, and whether it is closed. A closed
curve joins its last point back to its first. Each type keeps the data its own editing needs, so
nothing is lost by saving one, and each converts to a basis curve for rendering.

## The four types

| Type | Holds | Passes through its points |
|---|---|---|
| `Poly` | Points, joined by straight lines | Every one |
| `Bezier` | `BezierPoint`s: a position, a left and a right handle, and a `Handle` type for each | Every one; the handles shape the segments between |
| `CatmullRom` | Points, each segment shaped by the points either side of it | Every one |
| `Nurbs` | Points, an order, optional weights and optional knots | Only the first and last, when the knots are clamped |

- **Catmull-Rom** is uniform. An open curve's end points are their own neighbours beyond the ends,
  so the curve starts and ends on them.
- **NURBS.** The order is the degree plus one: 2 is straight lines and 4 is cubic.
  - Weights are empty, when every point weighs 1, or one positive weight per point. Unequal
    weights make the curve rational, and a rational curve can draw an exact circle.
  - Knots are empty, or the number of points plus the order, never falling. Empty knots are
    worked out: uniform and clamped on an open curve, so it starts and ends on its end points, and
    uniform on a closed one. A closed curve cannot be given knots of its own.

`problem(curve)` says why a curve cannot be evaluated, or gives an empty string. Every type needs
two points, a NURBS curve as many as its order, and every point and handle finite.

## Bezier handles

A `Handle` type says how a handle is placed:

| Type | Placed |
|---|---|
| `Free` | Where it was put, in any direction |
| `Aligned` | Where it was put, in line with the point's other handle |
| `Vector` | A third of the way toward the neighbouring point on its side |
| `Automatic` | Along the line from the point before to the point after, a third of the distance to the neighbour on its side, so the curve is smooth there |

- `settle(bezier, closed)` places every `Vector` and `Automatic` handle from the points. At an
  open curve's ends, the handle facing away from the curve shapes nothing and mirrors the one
  facing into it, and an automatic handle there points at its neighbour as a vector one does.
- `moveHandle(bezier, point, side, position)` moves one handle as a user dragging it does. A
  moved `Vector` handle becomes `Free`. A moved `Automatic` handle becomes `Aligned`, and so does
  the other handle when it is `Automatic`. When both are `Aligned`, the other turns to stay in
  line on the far side of the point and keeps its length.

## Evaluation

`evaluate(curve, resolution)` gives the points a curve passes through, for drawing and picking.

- Each segment is cut into `resolution` pieces, evenly in its parameter, so the list starts on
  the curve's start and has `resolution` more points per segment.
- A poly curve gives its points as they are.
- A NURBS segment is a span between two knots. Spans of no length are skipped.
- A closed curve ends with its first point again.

## The geometry

`Curves` ([api/curve/Curves.h](../../api/curve/Curves.h)) is any number of curves, each of its own
type, as one piece of geometry. A curve item in the editor holds one.

- `add(curve)` and `set(index, curve)` refuse a curve that `problem()` finds fault with, and
  return why. A Bezier curve's handles are settled as it is stored.
- `evaluated(index)` gives a curve's points at the geometry's `resolution()`, which is 12 by
  default. They are worked out when first asked for and kept until the curve or the resolution
  changes. Asking changes what is kept, so two threads must not read one `Curves` at once.
- `changes()` counts every add, replacement, removal and change of resolution. It only goes up,
  so a holder that remembers it can tell whether the geometry has changed since.

## Basis curves

A renderer takes curves as basis curves: one list of points under one basis, as RI's `Curves`
and USD's `BasisCurves` hold them. `BasisCurve` ([api/curve/Basis.h](../../api/curve/Basis.h))
has a `Basis` (`Linear`, `Bezier` or `CatmullRom`), whether it is periodic, and its points.

- A periodic curve joins its end to its start.
- A Bezier basis curve has three points per segment. A nonperiodic one has one more, and a
  periodic one's last segment ends on the first point.
- A Catmull-Rom basis curve runs from its second point to its second to last, each segment
  shaped by the points either side.

`toBasis(curve, pieces)` converts a curve:

| From | To | Exact |
|---|---|---|
| Poly | Linear | Yes |
| Bezier | Bezier, the points and handles in order | Yes |
| Catmull-Rom | Catmull-Rom. An open curve gains a copy of each end point beyond the end, and a closed one leads with its last point | Yes |
| NURBS of order 2 | Linear, through the points at its knots | Yes, in shape |
| NURBS of order 3 or 4 with equal weights | Bezier, one segment per span | Yes |
| Any other NURBS | Bezier, `pieces` segments per span, each matching the curve's position and direction at its ends | No |

`exact(curve)` says whether the conversion is exact. `evaluate(basisCurve, resolution)` evaluates
a basis curve as `evaluate()` does a curve, so an exact conversion gives the same points.
