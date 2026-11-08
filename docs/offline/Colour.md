# Colour management

This page is for anyone changing how moya's new core names colours, converts between colour
spaces and shows a picture on a display. The code is host code, in `moya/libmoya/colour/` and
the namespace `v3d::moya::colour`. It goes through OpenColorIO, which libmoya links PRIVATE:
only `Config.cxx` and the headers in `colour/internal/` include it.

## The configuration

`colour::Config` is an OpenColorIO configuration.

- **With no argument it is the built-in ACES 2.0 CG configuration**,
  `ocio://cg-config-v4.0.0_aces-v2.0_ocio-v2.5`, which OpenColorIO compiles in. No configuration
  file ships. The name is pinned to one version, so a newer OpenColorIO does not change a
  picture.
- **A path gives another configuration**, from a file or by another built-in `ocio://` name. A
  configuration that cannot be read, or that fails OpenColorIO's validation, throws
  `std::runtime_error`.
- **The working space is the `scene_linear` role.** `workingSpace()` gives its name, ACEScg in the
  built-in configuration, and `workingInteropId()` its Color Interop Forum ID, `lin_ap1_scene`.

Background: [ADR-0198](../adr/offline/0198-offline-the-working-space-is-opencolorios-scene-linear-role.md)

## ACEScg inside, the working space outside

The film holds the beauty in ACEScg, and the RGB-to-spectrum table takes ACEScg, whatever the
working space. Every conversion is to or from ACEScg, through the configuration's interchange
roles, `aces_interchange` or `cie_xyz_d65_interchange`. A configuration without either can name
colours but cannot reach ACEScg, and a conversion from it throws `std::runtime_error`.

- **`input(space)`** converts a colour named in a space into ACEScg, before its spectrum is
  found. An empty name is the working space, so an untagged colour is read in it. A space the
  configuration lacks throws `std::runtime_error`.
- **`output()`** converts the film's ACEScg into the working space. An output written in the
  working space goes through it, and names the working space's ID in its header.
- **`display(display, view)`** converts the film's ACEScg into the working space, then through a
  display's view, for an 8-bit picture or the editor's render view. `defaultDisplay()` and
  `defaultView(display)` give the configuration's defaults: the sRGB display through ACES 2.0's
  SDR 100-nit Rec. 709 view, in the built-in configuration. A display or view the configuration
  lacks throws `std::runtime_error`.

## Transforms

A `colour::Transform` is one conversion, held as an OpenColorIO CPU processor at 32-bit floats.

- **`apply(rgb)`** converts one colour.
- **`apply(values, pixels, stride)`** converts the first three floats of each pixel of a run in
  place, and leaves the rest. A snapshot's beauty is the first three floats of each pixel, with
  the snapshot's stride.
- **A transform is safe to apply from any number of threads at once.**
