# rib

RIB, the RenderMan Interface Bytestream: its reader, writer and recorder, and the requests a
handler takes. moya reads it and the editor writes it. It uses no Vulkan and no SDL.

Its CMake target is `v3dlib_rib`. A project outside the tree links it as `v3d::rib`.

- [offline/Rib.md](../../docs/offline/Rib.md) describes it, with moya's reading of each request.
- Its classes and functions are in the [API reference](https://farrcraft.github.io/vertical3d/reference/namespacev3d_1_1rib.html).
- [api/README.md](../../docs/api/README.md) lists every library and what each one links.
