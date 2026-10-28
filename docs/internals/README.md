# Library internals

These pages are for someone changing a library under `api/` rather than using it. Each assumes
the page that tells an app how to use the same library.

| Document | Covers | Using the library |
|---|---|---|
| [realtime/](realtime/README.md) | How `api/render/realtime` works: frames in flight, recording, memory, pipelines and descriptor sets | [api/rendering/](../api/rendering/README.md) |
| [ui/](ui/README.md) | How `api/ui` works: layout, drawing, hit testing, input routing and the immediate layer | [api/ui/](../api/ui/README.md) |

The offline renderer, `moya` and `api/render/offline`, is in [offline/](../offline/README.md),
which serves both kinds of reader. The other libraries have no internals page: the page that
describes how to use one also states its rules.
