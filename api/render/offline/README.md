# render_offline

What the offline renderer moya is built on: RIB, the shading language, sampling, the film and
the ray tracer. It uses no Vulkan and no SDL.

Its CMake target is `v3dlib_render_offline`. A project outside the tree links it as
`v3d::render_offline`.

- [offline/README.md](../../../docs/offline/README.md) describes it, with moya.
- [api/README.md](../../../docs/api/README.md) lists every library and what each one links.
