# Compositing

This page is for anyone changing or using what moya writes for a compositor beyond
the beauty: light path expressions, light groups, Cryptomatte and deep files. A scene asks for
each through its `channel` and `output` objects, as [Scene.md](Scene.md) describes. A RIB file
asks through `DisplayChannel`, `Display` and `PxrCryptomatte`, as [Rib.md](Rib.md) describes.
Alpha is in [Film.md](Film.md), the values at the first hit in
[Integrator.md](Integrator.md#values-at-the-first-hit), and the denoised channels in
[Output.md](Output.md#denoised-channels).

The code is in four places:

- `moya/libmoya/lpe/` compiles expressions.
- `moya/libmoya/kernel/integrate/` reports and records what a path does.
- `moya/libmoya/integrate/internal/` merges coverage and deep lists on the host.
- `moya/libmoya/output/` writes them.

## What a compositor gets

| Output | Asked for by | Written as |
|---|---|---|
| A light path expression | a channel of the source type `lpe` | a channel of three floats |
| A light group | a light's `lightGroup`, and a channel of an expression of its label | a channel of three floats |
| A Cryptomatte layer | a raw channel of the source `crypto_object`, `crypto_material` or `crypto_asset` | its ranks' channels, and its manifest in the header |
| A deep file | an output of the format `deepexr` | a deep scanline EXR file |

## The events of a path

A path reports each of its events as OSL's light path expressions name them, as a
`kernel::integrate::Event`: its type, how it scatters, and its label. The events are reported
beside what the path gathers, and change nothing it gathers.

| Type | Letter | Where a path reports it |
|---|---|---|
| Camera | `C` | first, as the camera sample is generated |
| Reflection | `R` | a bounce that leaves on the side of the surface it arrived from |
| Transmission | `T` | a bounce that crosses the surface |
| Light | `L` | each light of the scene whose light it gathers |
| Object | `O` | each emitting material whose light it gathers |

- **A bounce's scattering** is singular, `S`, for a sample from a closure that scatters into one
  direction. It is diffuse, `D`, for a sample from a diffuse, Oren-Nayar or fuzz closure, and
  glossy, `G`, for any other. The camera, a light and an object scatter none.
- **A bounce reports its event** once its depth limits allow it, before Russian roulette.
- **A light's event is reported wherever the path gathers its light.** That is at a light sample
  that brings some in, a light's shape the ray meets, and a mesh light at a shading point. It is
  also at each dome or distant light that a ray leaving the scene meets. A path goes on from the
  event before a light's, so a light it meets follows the bounce that sent the path to it.
- **A light sample is gathered closure by closure**, each part after the event of the bounce its
  closure would make towards the light, and then the light's. A diffuse wall lit by a light sample
  reports `<RD>` then `L`, as a bounce to the light would. Those events come after the bounce
  that arrived at the shading point and before the bounce that leaves it.
- **A surface that is cut out** reports no event where a ray passes through it.

`render()` fills an `integrate::Trails` when it is given one, for inspecting what paths do. It
holds the events of each pixel's sample zero, in the order they were reported, up to
`kernel::integrate::kTrailSteps` of them, with each light's event marked as gathered. It also
holds the names of the labels. The picture is the same as a render's without one. Sample zero
lies in the first pass, so a render that resumes past it records nothing.

Background: [ADR-0235](../adr/offline/0235-offline-a-bounce-samples-one-closure-alone.md)

## Light path expressions

`lpe::compile()` reads every light path expression of a render, in OSL's grammar, and compiles
them into one deterministic automaton, an `lpe::Automaton`. A path steps it once for each event,
and a state names each expression the events so far match.

- **Each event is one symbol**, which `kernel::integrate::symbolOf()` gives it. The symbols are
  the camera, a reflection and a transmission of each scattering, an object, and a light in no
  group. A light of each group follows, by its label, so a render of n groups reads 9 + n.
- **An event is written `<type scattering label>`.** Each place is a letter, a quoted label, `.`
  for any, or a set in brackets, `[...]`, or of all but them, `[^...]`. Places left off match
  anything. The types are `C`, `R`, `T`, `V`, `L`, `O` and `B`, and the scatterings `D`, `G`,
  `S` and `s`. Paths here have no volume, background or straight events, so `V`, `B` and `s`
  match none.
- **A letter alone is one event**: a type's letter is any event of that type, and a scattering's
  any event of that scattering. `RD` is a reflection and then a diffuse event, and a diffuse
  reflection is `<RD>`. A quoted label alone, such as `'key'`, is a light of that group, and `.`
  is any event.
- **Patterns follow one another**, are grouped in parentheses, and are alternatives across `|`.
  A set of events in brackets matches any of them, and `[^...]` any event but them. A pattern
  repeats by `*`, `+`, `?`, `{n}`, `{n,}` or `{n,m}`, a count at most `lpe::kMostRepeats`, 32.
  Spaces between parts are ignored.
- **A label no light group holds matches no light.** A light in no group carries no label.
- **An expression that cannot be read** matches nothing. `compile()` names it with the place,
  from zero, where reading it stopped, and why.
- **The automaton is built by subset construction**, visiting each state's symbols in order,
  and is then made minimal. Its states are numbered as a walk from the start first reaches them,
  so two sets of expressions that match the same paths give the same automaton.
- **A set is refused** with `lpe::Refused` when its automaton holds more than
  `lpe::kMostStates`, 1024, states while it is built.

### Channels of light path expressions

A channel whose `sourceType` is `lpe` and whose `sourceName` is an expression, of three
components, takes the light of the paths its expression matches, in ACEScg.

- **A render compiles its channels' expressions into one automaton**, under its light groups'
  labels, and the path state holds the path's state of it. A path steps it at the camera and at
  each bounce.
- **Each light the path gathers** steps a copy of that state by the light's event, or by a light
  sample's closure event and then the light's. The light, times the path's throughput, is added
  to the channel of each expression the copy accepts, by the channel's combine, with the
  sample's filter weight. The path's own state is left as it is.
- **The light is split as the beauty gathers it.** Channels that split every path one way add up
  to the beauty, to float rounding, and the beauty is the same with any channels or none.
- **A render takes the first `kernel::integrate::kMostExpressions`, 32, channels of
  expressions.** One past them, one of other components than three, one whose expression cannot
  be read, and every one of a set the automaton refuses is reported, and holds zero.

Background:
[ADR-0231](../adr/offline/0231-offline-light-path-expressions-run-as-one-automaton.md)

## Light groups

A light group lets a compositor change the light of some lights after rendering.

- **A light's label is its light group's**: the place of its `lightGroup` among the render's
  labels, which are the groups' names, sorted, each once. A light whose `lightGroup` is empty, and
  an object, carry none.
- **A light group's channel is an expression of its label**, such as `C.*<L.'key'>`. The light
  of the lights no group names is `C.*<L.[^'key''fill']>` for a render whose groups are `key` and
  `fill`, and an emitting material's is `C.*O`. Those channels split every path one way. A
  group's channel added to the beauty once more is the picture of its lights twice as bright.

## Cryptomatte

A Cryptomatte layer gives each pixel the IDs of the objects, materials or assets its samples met,
each with its share of the pixel. It is a raw channel of the source name `crypto_object`,
`crypto_material` or `crypto_asset`, as [Scene.md](Scene.md) says.

### Coverage

- **An ID names what a camera sample first met.** An object's ID is the name of the innermost
  instance that placed the surface, and a material's the name of the surface's material. An
  asset's is the name of the group the outermost instance places, or of that instance when it
  places a geometry. A sample that met nothing, or a light's shape, covers no ID.
- **A camera sample keeps one surface**, so a surface that is cut out is covered by the samples
  it stops, and glass takes its own ID.
- **Each camera sample records what it covers and its filter weight** in a buffer of the pass,
  `kernel::integrate::Covered`. A stopped pixel's records are empty.
- **After each pass, the host adds each pixel's records in sample order**, in
  `integrate::internal::Coverage`. Each ID's weights are summed, and so is the weight of every
  sample the pixel took. An ID's coverage is its weight over that whole weight. Nothing is lost
  however many IDs a pixel holds.
- **A layer keeps each pixel's IDs of the most coverage**, `ranks` of them, the lower ID first in
  a tie. They are written to the film channels `<layer>00`, `<layer>01` and on, two ranks to a
  channel of four floats: an ID, its coverage, an ID, its coverage. A rank of no ID is zero. Each
  ID is the float whose bits are its hash, as [its files](#files) says.
- **A snapshot holds each layer's manifest**, a JSON object of every name its IDs hold in the
  picture and each one's ID, by the layer's name in `manifests`.

Background:
[ADR-0232](../adr/offline/0232-offline-cryptomatte-coverage-is-merged-on-the-host.md)

### Files

An output of a Cryptomatte layer writes it as the Cryptomatte specification lays it out.

- **Its ranks are the channels `<layer>00.R` to `.A`, `<layer>01` and on**, of 32-bit floats,
  beside a beauty of the output's precision.
- **An ID is a name's 32-bit MurmurHash3**, `output::idOf()`, with the lowest bit of its exponent
  flipped when the exponent is all zeros or all ones. Its float is never a NaN, an infinity or a
  denormal.
- **The header names each layer under its key**, the first seven hex digits of the layer name's
  hash. It writes `cryptomatte/<key>/name`, `hash` as `MurmurHash3_32` and `conversion` as
  `uint32_to_float32`. Its `manifest` is a JSON object of each name in the picture and its ID, as
  eight hex digits.

## Deep output

### Deep lists

A render of a deep output keeps each pixel's samples in depth, which a deep file holds. Each
camera sample records, beside what Cryptomatte reads, how far along its ray it met anything and
the radiance it gathered, in ACEScg.

- **After each pass, the host joins each pixel's records in sample order**, in
  `integrate::internal::Deep`. A record joins the first entry of its object and material whose
  depth lies within the options' `deepTolerance`, a share of that entry's depth, of its own. It
  starts an entry otherwise. A record of no surface joins by its depth in the same way, and every
  record that met nothing joins one entry behind all the others.
- **An entry is a point sample at the depth of its first record.** A camera sample keeps one
  surface, so a surface that is cut out is a sample of its own, and a surface behind glass is part
  of the glass's sample.
- **A snapshot resolves each pixel's entries front to back.** An entry's alpha is its weight over
  the weight of the samples that reached its depth, and its colour its weighted radiance over that
  weight, premultiplied. Compositing a pixel's samples over each other gives its flat beauty and
  alpha. The entry of what met nothing has an alpha of zero, and is left out when it holds no
  light. A pixel no record reached holds no sample.
- **Each sample names its object's and material's IDs**, the MurmurHash3 of their names as
  Cryptomatte's IDs are, or zero for none.

Background: [ADR-0233](../adr/offline/0233-offline-deep-pixels-are-merged-on-the-host.md)

### Deep files

An output of the format `deepexr` writes the snapshot's deep samples, as
[Deep lists](#deep-lists) describes them, to a deep scanline EXR file with
`output::writeDeepExr()`.

- **Each sample is `R`, `G`, `B` and `A`**, its colour premultiplied by its alpha, at the
  output's precision, then `Z` and `ZBack`, both its depth, as 32-bit floats. A pixel's samples
  run front to back, so the file is tidy.
- **`objectid` and `materialid` are unsigned integers**, the IDs of OpenEXR's Deep IDs. The
  header's `idManifest` names each by MurmurHash3, the object's under `objectid` and the
  material's under `materialid`.
- **The header names the colour space** as `colorInteropID`, `lin_ap1_scene`.
- **A deep file is compressed by none, RLE or ZIPS**, which is all OpenEXR's deep files take. Any
  other compression the output asks for is written as ZIPS, ZIP the fallback among them.
