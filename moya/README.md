# moya

A RenderMan-style offline renderer. It reads a RIB scene file and writes an image by path
tracing. It links no Vulkan and no SDL, and it opens no window.

The renderer is `v3dlib_moya` in [libmoya/](libmoya/). It reads RIB through the api library
[api/rib](../api/rib/). `moya.cxx` is the command-line driver.
[tools/rgbtable/](tools/rgbtable/) fits the RGB-to-spectrum table libmoya compiles, and
[tools/albedo/](tools/albedo/) integrates the GGX albedo tables energy compensation reads, each
when moya is built.

- [docs/manual/moya/](../docs/manual/moya/README.md) is the manual for rendering with it: the
  command line, the RIB requests it reads and the shading language.
- [docs/offline/RunningAndTesting.md](../docs/offline/RunningAndTesting.md) covers building and
  testing it.
- [docs/offline/](../docs/offline/README.md) covers how a scene becomes an image, and each part of
  the renderer.
