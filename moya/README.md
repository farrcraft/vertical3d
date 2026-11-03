# moya

A RenderMan-style offline renderer. It reads a RIB scene file and writes an image, with a Reyes
hider or with ray tracing. It links no Vulkan and no SDL, and it opens no window.

The renderer is `v3dlib_moya` in [libmoya/](libmoya/), built on the api library
[api/render/offline](../api/render/offline/). `moya.cxx` is the command-line driver.

- [docs/offline/RunningAndTesting.md](../docs/offline/RunningAndTesting.md) covers running it,
  and building and testing it.
- [docs/offline/](../docs/offline/README.md) covers how a scene becomes an image, and each part of
  the renderer.
