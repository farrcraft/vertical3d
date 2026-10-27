# Assets and files

This page is for someone writing an app against the api. It covers loading files through the
asset manager, loading on a worker thread, the loaders, JSON documents, writing files safely and
upgrading old documents. Terms are defined in the [glossary](README.md#glossary).

What the loaders read is described with the type they read into: images in
[Images.md](Images.md), models and glTF in [Models.md](Models.md), and typefaces in
[Fonts.md](Fonts.md).

- [The asset manager](#the-asset-manager)
- [Loading on a worker thread](#loading-on-a-worker-thread)
- [Loaders](#loaders)
- [JSON documents](#json-documents)
- [Writing documents](#writing-documents)
- [Reading old documents forward](#reading-old-documents-forward)

## The asset manager

`v3d::asset::Manager` ([api/asset/Manager.h](../../api/asset/Manager.h)) loads files from one
directory. The game engine builds one rooted at `<app path>/data/` and returns it from
`Engine::assets()`. Every relative name an app loads resolves there, not against the working
directory.

```cpp
auto doc = assets()->load<v3d::asset::kind::Json>("level.json");
auto png = assets()->load<v3d::asset::media::kind::Image>("units.png");
auto model = assets()->load<v3d::asset::media::kind::Model>("knight.glb");
```

- `load<T>(name)` picks the loader from the file's extension, which is matched in lower case,
  and returns the asset as `T`. A file whose extension no loader reads, or that has none, is
  loaded by the loader registered for `T`.
- `load(name)` returns the asset as an `asset::Asset`, and needs an extension a loader reads.
- `path(name)` returns the full path a name resolves to, for code that opens the file itself.
- **Every failure gives a null pointer and a line in the log.** A missing file, a file that does
  not decode, an extension nothing is registered for, and a file of a different kind than `T`
  all fail the same way. Check the one pointer.
- The manager does not cache. Each call reads the file again, so keep what you load.
- **One manager can load on several threads at once.** Its loaders are only read after they are
  registered, and each load decodes with its own state. Register every loader before the first
  load on another thread.

To load from somewhere else, build a second manager on that directory, for example on
`engine::userPath()` for a saved game. Register the loaders it needs on it first.

## Loading on a worker thread

`asset::Worker` ([api/asset/Worker.h](../../api/asset/Worker.h)) runs jobs on one thread of its
own, in the order they are posted. Each job's result comes back to the thread that polls it.

```cpp
worker_.post(
    [assets = assets(), name]() { return assets->load<v3d::asset::media::kind::Image>(name); },
    [this, name](std::expected<boost::shared_ptr<v3d::asset::media::kind::Image>, std::exception_ptr> result) {
        if (result && *result) {
            upload(name, *result);
        }
    });

void MyApp::tick(unsigned int delta) {
    worker_.poll();  // runs every finished job's completion, here
}
```

- **A job runs on the worker thread.** It may read and decode files, through the app's manager
  or another. It must not call Vulkan, change an ECS registry, or use a realtime registry such
  as `MeshRegistry`, because the main thread uses all of those unguarded.
- **A completion runs inside `poll()`**, on the thread that calls it, in the order the jobs were
  posted. It receives a `std::expected` holding the job's result, or the exception the job
  threw. A job returns a value; it cannot return `void`.
- **Poll once a frame, from `tick()`.** A result therefore arrives at least one frame after its
  job is posted. `outstanding()` counts the jobs whose completions have not run, for a loading
  screen.
- **A completion that throws** stops the poll, and the exception reaches the caller. The
  completions after it run on the next poll.
- **Destroying a worker** drops the jobs that have not started and the completions that have not
  run. The job that is running finishes, and the thread is joined, so a long job delays the
  destructor.
- Whatever a job captures must outlive it. A job that captures `this` is unsafe unless the
  worker is destroyed before the object.

Background: [ADR-0086](../adr/0086-loading-one-worker-thread-polled-from-the-main-thread.md)

## Loaders

A manager loads only file types that have a loader registered on it. Each loader is registered
for the class of asset it produces, its kind, and a list of extensions.

| Registered by | Extensions | Returns |
|---|---|---|
| Every `Manager`, in its constructor | `.json` | `asset::kind::Json` |
| | `.txt` | `asset::kind::Text` |
| `asset::media::registerLoaders()` (`v3d::asset_media`) | `.png`, `.jpg`, `.jpeg`, `.tga`, `.bmp` | `asset::media::kind::Image` |
| | `.gltf`, `.glb` | `asset::media::kind::Model` |
| `audio::registerLoaders()` (`v3d::audio`) | `.wav` | `audio::kind::Sound` |

- **The game engine registers the media loaders on the manager it builds.** It does not register
  the audio loader; an app that plays sound calls `audio::registerLoaders()` itself. See
  [engine/Audio.md](engine/Audio.md#audio).
- **A manager you build yourself**, in a test or a tool, has only the JSON and text loaders.
  Call `registerLoaders()` for the others. Forgetting is a failed load at run time, not a link
  error.
- **A typeface is not an asset.** The size a face is rasterized at belongs to the caller, so
  `ui::paint::TextRenderer` opens the face itself at the path `Manager::path()` gives.

To add a loader, derive from `asset::Loader` ([api/asset/Loader.h](../../api/asset/Loader.h)),
implement `load(name)`, and register it:

```cpp
manager.registerLoader(boost::make_shared<MyLoader>(logger), {".lvl"});
```

- A loader passes its kind to the `Loader` constructor, as `typeid(MyLevel)`, and every asset
  it returns is of that class. Any library or app can add a kind.
- Extensions include the dot and are lower case. An extension registered again moves to the new
  loader.
- **A second loader of one kind** keeps the extensions it was registered with, and takes over the
  files whose extension no loader reads. The first keeps its own extensions.
- **`load()` returns null and logs on failure. It never throws.**
- **A loader holds no state between calls.** One instance serves everything that loads through
  the manager, possibly on several threads at once, so a setting left on it by one caller would
  be seen by the next.

Splitting the loaders this way keeps each library's dependencies small. A library that reads only
documents needs only the JSON loader, so it does not pull in libpng, libjpeg or FreeType. Only an
app that links `audio` pulls in SDL3_mixer.

Background: [ADR-0079](../adr/0079-assets-loaders-are-registered.md),
[ADR-0118](../adr/0118-assets-a-loader-is-keyed-by-the-kind-it-produces.md)

## JSON documents

`asset::kind::Json::document()` returns the parsed `boost::json::object`. Read its members
through the checked reads in [api/asset/Json.h](../../api/asset/Json.h). `boost::json::object::at()`
throws on a missing key, and `boost::json::value_to` and the `as_` accessors throw on a value of
the wrong type. When the document is not what you expect, return false with a log line.

**A game's own text format keeps its parser in the game.** The api parses the formats its own
libraries read: JSON documents, glTF, and RIB for moya. A grammar added for one game's record or
script files would commit the api to keeping that format stable for games that do not use it.

Each checked read takes an object and a key:

| Read | Returns | Refuses |
|---|---|---|
| `readString(object, key)` | `std::optional<std::string>` | anything but a string |
| `readNumber(object, key)` | `std::optional<double>` | anything but a number |
| `readInt(object, key)` | `std::optional<int>` | anything but an integer an `int` can hold |
| `readBool(object, key)` | `std::optional<bool>` | anything but `true` or `false` |
| `readObject(object, key)` | `const boost::json::object*` | anything but an object |
| `readArray(object, key)` | `const boost::json::array*` | anything but an array |

- **A missing member and a member of the wrong type are both refused.** The read returns an
  empty optional or a null pointer, and does not throw for either.
- **A caller that reports the two cases differently tests `contains()` first**, then reads.
- `readNumber` takes a signed integer, an unsigned integer or a double, and returns a double. An
  integer beyond 2^53 loses precision. The value is not tested for being finite.
- `readInt` takes a signed or an unsigned integer an `int` can hold. It refuses a double, even a
  whole one, and an integer beyond the range of an `int` rather than narrowing it.
- `readBool` does not take a number as a bool.
- `readObject` and `readArray` return a pointer into the object rather than a copy. The pointer
  is valid while the object is unchanged.
- An element of an array is not a member, so these reads do not apply to it. Test its kind with
  `is_string()` or `is_number()` before reading it.

A number that becomes an integer goes through the checked conversions in
[Types.md](Types.md#checked-conversions) after it is read.

`asset::readFile(path)` in [api/asset/File.h](../../api/asset/File.h) reads a whole file as
bytes, without a loader. It sizes the file, then reads it. It returns `std::nullopt` when the
file will not open, cannot be sized, or reads fewer bytes than its size. An empty file gives an
empty string.

## Writing documents

The write functions are in [api/asset/Writer.h](../../api/asset/Writer.h).

| Function | What it does |
|---|---|
| `writeDocument(path, value)` | Writes a JSON value as readable text, ending in a newline. |
| `writeFile(path, bytes)` | Replaces a file with bytes. Use it for any non-JSON format. |
| `serializeDocument(value)` | Returns the readable text without writing it, with no trailing newline. |

**Every write is atomic.** The bytes go to a temporary file beside the target, which is then
renamed over it. If anything fails before the rename, the old file is untouched, the
temporary is removed, and the function returns false.

- The process needs permission to create files in the target's directory, not only to write the
  target.
- On Windows, renaming over a file that another process holds open fails. The function returns
  false; report it so the player can close whatever holds the file.
- The write is not flushed to disk. It is safe against a crash, not against power loss: after a
  power cut the player finds either the old document or the new one, never half of one.
- A temporary left by a process killed during a write is not cleaned up, but the next successful
  write to the same path overwrites it.
- Other programs watching the directory see a file created and renamed, not modified.

**The output is always readable, and there is no compact mode.** Scalars, short vectors, and
objects of those stay on one line. Everything else is indented two spaces per level. A double
that a float holds exactly prints as that float, so `0.1F` prints as `0.1` rather than
`0.10000000149011612`. Any other double prints as a double, so `1e-50` and `123456789.123` keep
their values. JSON has no form for infinity or NaN, so `writeDocument()` returns false for a
document holding one and leaves the file untouched. Call `asset::serializeDocument()` by its full
name. An unqualified `serialize()` on a `boost::json::value` finds `boost::json::serialize` by
argument-dependent lookup, and that writes the whole document on one line.

What a document holds is the caller's decision. These functions decide only how it reaches the
disk. `engine::Settings`, the editor's project manifests and `asset::media::SpriteSheets` all write
through them. The editor's scene documents are SQLite databases, written in a transaction rather
than by a rename. See [editor/Files.md](../editor/Files.md#scene-documents).

Background: [ADR-0041](../adr/0041-files-write-documents-atomically.md)

## Reading old documents forward

A document a game writes outlives the build that wrote it. `asset::readForward()` in
[api/asset/Migration.h](../../api/asset/Migration.h) upgrades an old JSON document to the
current version, one version at a time.

- **The document carries a whole number `"version"` at its root**, starting at 1.
- **The chain is a list of steps, oldest first.** `chain[0]` takes version 1 to 2, `chain[1]`
  takes 2 to 3, and so on. A build at version `n` has `n - 1` steps.
- A step is an `asset::Migration`, `std::function<bool(boost::json::object&)>`. It returns false
  if it cannot upgrade the document.

```cpp
const std::vector<v3d::asset::Migration> chain = {
    [](boost::json::object& doc) { doc["volume"] = doc["sound"]; doc.erase("sound"); return true; },
};
switch (v3d::asset::readForward(&doc, 2, chain)) {
    case v3d::asset::Reading::Current:  break;                 // already version 2
    case v3d::asset::Reading::Migrated: save(doc); break;      // may be written back
    case v3d::asset::Reading::Newer:    readOnly_ = true; break;
    case v3d::asset::Reading::Refused:  useDefaults(); break;
}
```

| Reading | Meaning |
|---|---|
| `Current` | Already at this build's version. Untouched. |
| `Migrated` | Upgraded to this build's version. It may be written back. |
| `Newer` | Written by a later build. Untouched. **Do not overwrite it.** |
| `Refused` | No version, a version that is not a whole number of at least 1, a missing step, or a failed step. Untouched. |

The rules:

- The upgrade runs on a copy and stamps the version after each step. The caller's document is
  replaced only if every step succeeds, so a document is never left half-upgraded.
- **A step must read and write only the document it is given.** `readForward()` cannot undo a
  change a step makes anywhere else.
- **Refusing to overwrite a `Newer` document is the caller's job.** `engine::Settings` does it
  through `writable()`.
- A step is plain code with no schema behind it. Write a test for each one.
- A step can never be removed while documents of the version before it may still exist.

Background: [ADR-0073](../adr/0073-files-migrate-old-documents-one-version-at-a-time.md)
