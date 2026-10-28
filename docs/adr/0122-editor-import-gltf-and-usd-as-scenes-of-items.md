# ADR-0122: Editor: import glTF and USD as scenes of items

**Status**: accepted
**Date**: 2026-10-27
**Documented in**: [editor/Files.md](../editor/Files.md)

## Context

The editor's scene holds meshes, cameras, lights and groups in a hierarchy, with keyed
properties, and edits meshes as polygons. A file brought in from another tool should arrive in
that shape, as items the user can select, move and key, rather than as one merged mesh. The
formats current tools exchange are glTF, USD, FBX, Alembic and OBJ. glTF stores triangles only,
so a modeller's quads cannot be recovered from it, while USD keeps polygons and also describes
curves. The api already reads glTF, for the games, into one merged model with no hierarchy,
cameras or lights.

## Decision

The editor imports glTF and USD. An import walks the file's hierarchy and makes an item for each
node: a group, a mesh, a camera or a light, placed and parented as the file places it, with its
animation as keys. LightWave, DXF, 3DS and MilkShape leave the Import menu.

## Alternatives

### The four legacy formats, each with a reader of its own
- **For**: keeps the menu as drawn.
- **Against**: no current tool writes LightWave, MilkShape or DXF meshes, and none of the four
  carries a scene the editor can hold.
- **Rejected because**: an importer is worth what people can bring through it.

### glTF only
- **For**: one format, the one the api and the games already read.
- **Against**: every import arrives as triangles, and joining coplanar ones recovers only flat
  polygons.
- **Rejected because**: a polygon modeller needs a format that keeps polygons.

### glTF and OBJ
- **For**: OBJ keeps polygons, and nearly every tool writes it.
- **Against**: OBJ carries geometry only, with no hierarchy, cameras, lights or animation.
- **Rejected because**: USD keeps polygons and carries the whole scene, curves included.

### Every format through Assimp
- **For**: the widest coverage, the four legacy formats and FBX included, through one library.
- **Against**: every format is reduced to Assimp's common scene, which loses what it does not
  model, and its glTF reader would sit beside the api's own.
- **Rejected because**: fidelity to the file matters more than the number of formats.

### Import glTF as the api's merged model
- **For**: the loader exists, and an import is one mesh.
- **Against**: the hierarchy, the cameras, the lights and the separate objects are all lost.
- **Rejected because**: an editor's import should keep what the file says about the scene.

## Consequences

- **Gains**:
  - Scenes from Blender, Houdini and Maya arrive with their hierarchy, cameras, lights and keys.
  - USD meshes keep their polygons, and USD can bring curves once the scene holds them.
  - The menu offers only formats current tools write.
- **Costs**:
  - OpenUSD is the largest dependency in the tree. It brings TBB, ships as shared libraries,
    and finds its plugins through files that must sit beside the editor.
  - Two scene readers, one per format, each mapping its file's model onto the editor's items.
  - FBX and OBJ files must go through another tool first.
  - glTF meshes still arrive as triangles.
- **Revisit when**: a format the editor cannot read becomes the common way to hand over a scene,
  or USD's dependency outgrows what the editor gains from it.
