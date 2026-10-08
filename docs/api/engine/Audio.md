# Audio

Sound through `api/audio`, which plays clips with SDL3_mixer, and events from banks through an
optional backend.

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
- `has(id)` says whether a clip is filed under an id. It works without a device.
- `busGain(bus, level)` sets the volume of a named bus such as `music` or `sfx`. It may be set
  before anything plays on that bus.
- **A device that does not open leaves the app silent, not broken.** `initialize()` returns
  false and logs, clips still load, and playing returns false.

The loader is registered for `.wav`.

## Events and parameters

Sound authored in an audio tool comes as banks of named events, with global parameters that the
mix reads. `audio::events(logger)` returns the event backend the build has:

```cpp
events_ = v3d::audio::events(logger());                  // in start()
events_->bank(path + "/Master.bank");
events_->bank(path + "/Master.strings.bank");
events_->play("event:/UI/Cancel");                       // a one shot
events_->parameter("Intensity", 0.8f);                   // a global parameter
events_->update();                                       // once a frame, in tick()
```

- **The backend is FMOD Studio when the build has it, and the null backend otherwise.** A build
  has FMOD only when it was configured with `V3D_FMOD_ROOT`; see
  [Dependencies.md](../../contributing/Dependencies.md#fmod-studio). The null backend plays
  nothing.
- **Every call says whether it did anything.** `bank()`, `play()` and `parameter()` return false
  for a bank that did not load, an event no loaded bank holds, or a parameter that does not
  exist, and always on the null backend. A game treats false as silence, not as an error.
- **Load the strings bank** beside the banks that hold events. FMOD looks an event's path up in
  it, so without it no event is found by name.
- `play()` starts a one shot, which FMOD releases when it ends.
- **Events and clips are separate.** Clips, buses and fades stay on `audio::Engine` and
  SDL3_mixer, whichever event backend is built. With FMOD, the two each open their own device.
- `name()` is `"fmod"` or `"null"`, for the log.

Background: [ADR-0021](../../adr/0021-audio-use-sdl3-mixer.md),
[ADR-0079](../../adr/0079-assets-loaders-are-registered.md),
[ADR-0084](../../adr/0084-audio-events-and-parameters-behind-an-interface.md)
