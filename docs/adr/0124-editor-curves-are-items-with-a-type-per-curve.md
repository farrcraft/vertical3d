# ADR-0124: Editor: curves are items with a type per curve

**Status**: accepted
**Date**: 2026-10-27
**Documented in**: [editor/Scene.md](../editor/Scene.md#the-scene)

## Context

The editor needs curves to draw by hand, to revolve, loft and sweep into meshes, to trim the
NURBS surfaces it will grow, as paths that items follow, and as hair. Each use is served by a
different form: Bezier curves whose handles keep a type, NURBS curves with weights for exact
circles and for surfaces, Catmull-Rom curves for hair, and polylines for traced paths.
Renderers and USD's hair schema take basis curves, which are points under one basis and cannot
keep a handle's type or a NURBS weight. A half-edge mesh cannot hold an edge that bounds no
face. USD files carry curves both as basis curves and as NURBS curves.

## Decision

A curve item is a kind of scene item, holding shared curve geometry from a library in the api.
The geometry holds any number of curves, each with its own type: poly, Bezier with a type on
every handle, Catmull-Rom, or NURBS with an order, weights and knots. Drawing and picking read a
cache of evaluated points, and the export converts each curve to basis curves.

## Alternatives

### Basis curves as the stored form
- **For**: what is edited is what RI and USD's basis curves hold, so the export and a hair
  import are lossless, and there is one evaluator per basis.
- **Against**: a Bezier handle's type and a NURBS curve's weights are lost on every save, so
  editing behaviour cannot persist and a circle is never exact.
- **Rejected because**: it is a form for rendering and hair, and the editor also models.

### NURBS as the one stored form
- **For**: one set of maths, which represents every other type and exact conics, as Maya does.
- **Against**: handle types and Catmull-Rom exist only inside the editing tools and are lost on
  save, and hair carries knots and weights it never uses.
- **Rejected because**: a type the user works in has to survive a save.

### One type for every curve in an item
- **For**: one evaluator per item, as USD and Maya each hold one type per prim or shape.
- **Against**: an outline that mixes straight runs and Bezier runs, or a groom with a few NURBS
  guides, becomes several items, and joining curves of different types needs a conversion.
- **Rejected because**: drawing and picking read evaluated points, so a mixed item costs only
  the evaluators that already exist.

### Curves inside the mesh geometry
- **For**: one geometry type for everything, as Houdini mixes curve and polygon primitives.
- **Against**: `brep::BRep` is a half-edge mesh, and a curve there makes every mesh algorithm
  handle edges that bound no face.
- **Rejected because**: it puts a curve's cost into every mesh operation.

## Consequences

- **Gains**:
  - Each use keeps the data its editing needs, and a save keeps it too.
  - Revolving, lofting and NURBS surfaces start from exact curves.
  - Both USD curve schemas import without loss.
  - Drawing, picking and the export read one list of evaluated points per curve.
- **Costs**:
  - Four evaluators, and every tool that edits control points handles each type.
  - A weighted NURBS curve is approximated by the export, because RI's `Curves` has no weights.
  - Changing a curve's type changes its shape, as it does in Blender.
- **Revisit when**: the renderers the editor targets take NURBS curves directly, or a type is
  wanted that a per-curve type cannot describe, such as a type per segment.
