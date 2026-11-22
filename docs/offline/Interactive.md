# Interactive rendering

This page is for anyone changing how moya renders a scene while clients edit it, and how it makes
pictures as it samples. It is in `moya/libmoya/integrate/`, in the namespace
`v3d::moya::integrate`: `Session.h`, with `Pictures.h`, `Picture.h` and `Built.h`. A batch
render is a session that takes no edits, so this is also the path `render()` and `moya` take.
[Integrator.md](Integrator.md) covers what one pass of samples does.

The editor's interactive render, its render view and its rendered views drive a session through
`MoyaDelegate`, as [the editor's Rendering.md](../editor/Rendering.md) says.

## A render session

`integrate::Session` renders a scene while clients edit it. It keeps every structure it builds
from one version of the scene to the next, and rebuilds only what the edits touched.

- **A session owns its scene.** It applies the scene's batches and reads it, so while a session
  lives nothing else applies or reads the scene. Clients edit and commit from any thread, as
  [Scene.md](Scene.md#edits-commits-and-versions) says.
- **`advance()` applies, rebuilds and runs one pass.** It applies every batch committed since the
  last. At a new version it brings the structures up to it, and starts the film again at the
  first sample. It gives false once the film holds every sample the version takes, every pixel
  has stopped, or the checkpoints' `stop` answered true.
- **A pass never sees half a batch.** A batch committed during a pass is applied after it, so
  its samples are not in the pass. `edited()` says that a batch was committed. A pass in progress
  then stops before its next tile and is dropped, so the film holds the passes before it, and the
  next `advance()` applies the batch.
- **An edited session ends where a fresh render does.** A session that took edits, and sampled to
  its end, gives the bytes in every channel that `render()` gives for the edited scene.
- **Every new version starts the film again**, even one whose edits change nothing a picture
  shows.
- **A session forgets the scene's removals** up to each version it builds, so the scene's record
  of them does not grow while the session lives.
- **It runs on a thread of its own, or on its caller's.** `advance()`, `picture()` and the counts
  are called by one thread while no thread of the session's runs. `start()` runs passes on a
  thread of the session's, which waits for `edited()` once a version has every sample. `wait()`
  returns once that thread runs no pass and nothing was said since. `stop()` drops the pass in
  progress and ends the thread. What the thread threw is thrown again by `wait()` and `stop()`.
- **`render()` is a session that takes no edits.** It never applies the scene, and runs
  `advance()` until it gives false. Its overload that takes prototypes builds one assembly over
  them in place of the assembler.

Background: [ADR-0253](../adr/offline/0253-offline-a-render-is-a-session-that-takes-edits-between-passes.md)

## What a build keeps

At each new version a session brings its structures up to the scene. It keeps what the edits since
the last build left as it was.

- **What is kept.** The hierarchies, through an `accel::Assembler`
  ([Traversal.md](Traversal.md#rebuilding-what-changed)), the compiled materials, the textures
  ([Textures.md](Textures.md#textures-kept-across-versions)), the volumes' blocks and the lights
  ([Lights.md](Lights.md#lights-kept-across-versions)). The tracing view and the records of the
  outputs are made again at each version.
- **The surfaces' buffers are kept when the prototypes are.** They are made again unless every
  edit since the last build was to a camera, and the gather built no geometry or group anew and
  compiled no material. A session given its prototypes makes them once.
- **The pixel filter is kept unless the options changed.**
- **The film keeps its storage** while its resolution and channels are the same, and is emptied.
  It is made again when either changes. `madeFilm()` says which the last build did.
- **`lastBuild()` says what the last build made again, and how long each part took**, as an
  `integrate::Built`. It gives the seconds of the geometry, the surfaces, the volumes, the
  textures, the lights, the pixel filter and the film. It gives the geometries and groups whose
  hierarchies were built, the materials compiled, and the lights made. It says whether the light
  tree was built, the volumes laid out, and the surfaces, the filter and the film made. A camera
  move makes nothing again but the film's sampling.

## Pictures as a session samples

`Session::show(pictures)` makes the session hand its caller an `integrate::Picture` as it samples.
A picture is the film's beauty through a display and its view, at 8 bits a channel, as
`output::displayed()` gives it. It names the scene's version it shows, the samples its pixels
hold, the samples they hold once the version has them all, and the side of the block each sample
stands for.

- **Coarse pictures come first after each new version.** Each renders one sample for each block of
  pixels, into a film of its own, and every pixel of the picture takes its block's colour. The
  first block is the least power of two that leaves the window's longer side at most `coarsest`
  blocks, 64 unless the caller gives another. Each coarse pass halves it, until the next would be
  a pixel. A window no longer than `coarsest` has none. Their samples are never added to the
  film.
- **The full passes then grow.** They take one sample a pixel, then one, two, four and eight, so
  the film holds twice the samples after each, and `kSamplesPerPass` after that. The film is the
  same as one sampled in whole passes, so an edited session still ends where a fresh render does.
- **A picture follows a pass** once `interval` seconds have passed since the last picture. The
  first pass of each version and the pass that fills the film always give one. A session that
  ends with samples no picture shows, by adaptive sampling or the checkpoints' `stop`, gives one
  as it ends.
- **The last picture is the render's.** Its bytes are those `output::displayed()` gives for the
  snapshot `render()` makes of the same scene.
- **Pictures are made on the thread that runs the passes**, which is the session's own after
  `start()`, and `shown` is called there. An edit drops a coarse pass as it drops any pass, and
  the dropped pass gives no picture.
- **A session without `show()`** runs no coarse passes, and samples in whole passes.

Background: [ADR-0254](../adr/offline/0254-offline-first-pictures-are-coarse-then-passes-grow.md)

## Denoising while sampling

- **A picture can be denoised beside the raw one**, once `denoised` is set and its pixels hold
  `denoiseFrom` samples, 16 unless the caller gives another. A coarse picture is not denoised.
  The guides are the albedo and normal the film holds, as
  [Output.md](Output.md#denoised-channels) says, so a scene that asks for no denoising gives
  none, and its beauty is denoised alone. A picture is denoised at OIDN's fast quality from the
  guides as they are. The last picture of a version is denoised as a batch render denoises its
  beauty, and its bytes are those of `beauty_denoised`. A version that adaptive sampling ends
  after a preview gives its last picture again, denoised so. The raw picture's bytes are the
  same whether pictures are denoised or not.
- **A picture can show the guides**, the albedo and the normal the film holds, once `guides` is
  set. The albedo is shown as the beauty is, and the normal is each component from -1 to 1 as 0
  to 255 with no display transform, as `output::directions()` gives it. A coarse picture shows
  neither, and nor does a picture of a film that holds no guides. The raw picture's bytes are the
  same whether the guides are shown or not.

Background: [ADR-0255](../adr/offline/0255-offline-each-picture-is-denoised-once-it-holds-enough-samples.md)

## Stopping and resuming

A session stops between tiles, through the checkpoints' `stop` or an edit, as
[Devices.md](Devices.md#a-launch-that-stops) and [Film.md](Film.md#passes-and-snapshots) describe.

- **`again()` makes the latest picture again** as `show()` now asks for, without a pass, so that
  a caller that changes what its pictures show once sampling has ended has a picture of it. The
  picture of a version that has ended is denoised as the last of a version is. It makes nothing
  before `show()` or before the version's first full pass.
- **`resume()` lets a version that the checkpoints' `stop` ended sample on**, from the passes its
  film holds, once the stop answers false again. The film it ends on is that of a version never
  stopped. A version that holds every sample stays ended.
- **A checkpoint describes the version the session renders**, counted from the edit that last
  started the film. A session resumes from its checkpoint only when it first builds.
