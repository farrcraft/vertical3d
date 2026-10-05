# Audio

Sound through `api/audio`, which plays clips with SDL3_mixer.

## Audio

`api/audio` plays sound through SDL3_mixer. The engine does not create an audio engine or link
the library, so an app that plays sound does three things in `start()`:

```cpp
// link v3d::audio, then:
sound_ = boost::make_shared<v3d::audio::Engine>(logger(), dispatcher());
v3d::audio::registerLoaders(*assets(), logger());      // the .wav loader
sound_->initialize();                                  // open the audio device
if (const boost::json::object* sounds = document(v3d::config::Type::Sound)) {
    sound_->load(*sounds, *assets());
}
```

The sound document lists clips by id:

```json
{ "sounds": [ { "clip_id": "hit", "file": "hit.wav" } ] }
```

- **A clip's file resolves through the asset manager**, relative to `data/`, like every other
  asset. The manager must have had `audio::registerLoaders()` called on it. A clip whose source
  is not a file can be loaded with the overload of `load()` that takes a `Resolve` callback.
- To play a one-shot from anywhere, publish a sound event:
  `dispatcher()->trigger(v3d::event::kind::Sound("hit"))`. The audio engine listens for it.
- `playClip(id)` also plays a one-shot. `play(id, how)` returns a `Voice` for a sound you need
  to control. `audio::Play` sets the bus, the loop count (-1 loops until stopped), a fade-in and
  a gain.
- `stop(voice, fadeOutMs)`, `stopAll()`, `playing(voice)` and `gain(voice, level)` control
  playing sounds. A voice that has finished is refused, never confused with a newer sound.
- `busGain(bus, level)` sets the volume of a named bus such as `music` or `sfx`. It may be set
  before anything plays on that bus.
- **A device that does not open leaves the app silent, not broken.** `initialize()` returns
  false and logs, clips still load, and playing returns false.

The loader is registered for `.wav`.

Background: [ADR-0021](../../adr/0021-audio-use-sdl3-mixer.md),
[ADR-0079](../../adr/0079-assets-loaders-are-registered.md)
