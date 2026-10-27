# Audio

Sound through `api/audio`, which plays clips with SDL3_mixer, and events from banks through an
optional backend.

## Audio

`api/audio` plays sound through SDL3_mixer. The engine does not create an audio player or link
the library, so an app that plays sound does three things in `start()`:

```cpp
// link v3d::audio, then:
sound_ = boost::make_shared<v3d::audio::Player>(logger(), dispatcher());
v3d::audio::registerLoaders(*assets(), logger());      // the .wav loader
sound_->initialize();                                  // open the audio device
if (const boost::json::object* sounds = document(v3d::audio::Player::DOCUMENT)) {
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
  `dispatcher()->trigger(v3d::event::kind::Sound("hit"))`. The audio player listens for it. A
  second argument pans it: `Sound("hit", -0.5F)` sounds left of centre. A position in its place
  places it: `Sound("hit", glm::vec3(2.0F, 0.0F, -5.0F))`.
- `playClip(id)` also plays a one-shot. `play(id, how)` returns a `Voice` for a sound you need
  to control. A `Voice` is a scoped enum, so it cannot be passed where a fade or a level goes.
  `Voice::None` is no sound, which a failed `play()` returns. `audio::Play` sets the bus, the
  loop count (-1 loops until stopped), a fade-in, a gain, and a pan or a position.
- `stop(voice, fadeOutMs)`, `stopAll()`, `playing(voice)`, `gain(voice, level)`,
  `volume(voice)`, `pan(voice, value)` and `position(voice, point)` control playing sounds. A voice that has finished is
  refused, never confused with a newer sound.
- **A pan runs from -1, left, through 0 to 1, right.** The side a sound is panned towards stays
  at full gain, and the other falls in a straight line to silence at the end. A centred sound is
  exactly an unpanned one. A pan past either end is held there, and one that is not finite is
  centred. `audio::stereo(pan)` gives the two gains.
- **A sound is either panned or placed.** `Play::position` places a sound at a point in the
  world, and then its pan is not used. `pan(voice, value)` on a placed sound unplaces it, and
  `position(voice, point)` on a panned sound places it. SDL_mixer holds a track in one mode or
  the other, so there is no pan on top of a position.
- **A placed sound is heard from the listener.** `listener(Listener)` sets where sound is heard
  from: a position, the way it faces, its up and its right, in the same world coordinates as each
  sound's position. Call it once a frame from the camera or the player, from `tick()`. Every
  placed sound is heard from the new listener at once. A listener that is never set stands at
  the origin facing -z, with +y up and +x to its right.
- **A listener takes a camera's eye and three axes**: a `camera::Profile`'s `eye()`,
  `direction()`, `up()` and `right()` are its position, forward, up and right. Right is given,
  not worked out from the other two. The two `Profile::Hand` bases put opposite sides of the
  world on the right of the screen. A sound on the right of the screen is heard on the right in
  either.
- The three need not be unit length or at right angles: the engine makes them so, keeping
  forward's direction and the side right is on. A listener that faces no way is refused and the
  last one kept: a forward, an up or a right of zero, a forward and an up that are parallel, a
  right in their plane, or a value that is not finite.
- **A placed sound is quieter the further it is.** The fall-off is SDL_mixer's own and cannot be
  tuned. A placed sound is mixed in mono and spread across the speakers by its direction.
- `heard(voice)` reads back where the mixer holds a placed sound: relative to the listener, with
  x to the right, y up and z behind. It reads back nothing for a panned sound.
  `audio::relative(listener, point)` is the same conversion, with no device.
- `has(id)` says whether a clip is filed under an id. It works without a device.
- **A sound plays at its own gain times its bus's level.** `busGain(bus, level)` sets the
  level of a named bus such as `music` or `sfx`. It applies to the sounds playing on the bus and
  to every sound started on it later. `gain(voice, level)` sets a sound's own gain, and the
  bus's level still scales it. `volume(voice)` reads back the product.
- **A bus level is stored.** It can be set before `initialize()`, before anything plays and
  with no device, so a volume read from settings can be applied at any point in `start()`.
  `busGain(bus)` reads it back. A bus never set is at 1, and so is no bus.
- **Call `shutdown()` in the app's `release()`.** The engine quits SDL as soon as `release()`
  returns, before the app's members are destroyed, and a player that still holds a sound then
  faults as it is destroyed. `shutdown()` stops every sound and closes the device, and the
  destructor does nothing after it.
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
events_->parameter("Intensity", 0.8F);                   // a global parameter
events_->volume("vca:/Music", 0.5F);                     // a player's volume slider
events_->paused("bus:/Game", true);                      // a pause menu
events_->update();                                       // once a frame, in tick()
sound_->attach(events_);                                 // hear events from sound_'s listener
```

- **The backend is FMOD Studio when the build has it, and the null backend otherwise.** A build
  has FMOD only when it was configured with `V3D_FMOD_ROOT`; see
  [Dependencies.md](../../contributing/Dependencies.md#fmod-studio). A build with FMOD still
  gets the null backend when FMOD's dlls are not beside the executable, or FMOD fails to start.
  The null backend plays nothing.
- **Every call says whether it did anything.** `bank()`, `play()`, `parameter()`, `volume()`
  and `paused()` return false for a bank that did not load, an event no loaded bank holds, or a
  parameter or a mixer path that does not exist, and always on the null backend. A game treats false as silence, not as an error.
- **A missing event or parameter is logged once per name.** The FMOD backend warns the first time
  an event is not found or a parameter cannot be set, and returns false without logging on every
  later call for that name. A parameter set every frame writes one line, not one a frame. An
  event and a parameter with the same name are each reported.
- **Load the strings bank** beside the banks that hold events. FMOD looks an event's path up in
  it, so without it no event is found by name.
- **`volume(path, level)` sets a bus or a VCA**, told apart by the path: `"bus:/"` is the master
  bus, `"bus:/Music"` a bus under it, and `"vca:/SFX"` a VCA. A VCA scales the buses assigned to
  it without changing their routing, which is what a player's slider usually wants. 1 is as
  authored and 0 is silent, and a level below 0 is taken as 0. `volume(path)` reads the level
  back.
- **`paused(bus, on)` pauses a bus** and everything routed through it. `"bus:/"` pauses
  everything, and pausing the game's buses but not the menu's is a pause menu. A VCA has no
  pause, so a `"vca:/"` path is refused. `paused(bus)` reads it back.
- A bus or VCA path is found through the strings bank too, so a level or a pause set before it
  is loaded is refused. Set them again once the banks are loaded. A path no bank holds is refused,
  logged once per path, and reads back as nothing. On the null backend every one is refused.
- `play()` starts a one shot, which FMOD releases when it ends.
- **`start()` starts an event and keeps hold of it**, for a bed or a loop that has to be stopped.
  It returns an `audio::Instance`, a scoped enum like `Voice`. `Instance::None` is no event,
  which an unknown event and the null backend return. `stop(instance, Stop::FadeOut)` plays the
  fade out the event was authored with, and `Stop::Immediately` cuts it off. `playing(instance)`
  reads FMOD's playback state, and stays true through a fade out.
- **FMOD starts and stops events on its own thread**, so `playing()` changes a frame or more
  after `start()` or `stop()`. `update()` releases each started event once it has stopped, and
  a released instance is refused rather than confused with a newer one.
- An instance belongs to the backend that started it, and nothing stops it when its owner goes.
  An owner that ends before the sound should, such as a scene, stops it in its destructor.
- **Events and clips are separate.** Clips, buses and fades stay on `audio::Player` and
  SDL3_mixer, whichever event backend is built. With FMOD, the two each open their own device.
- **An event can be placed.** `play(event, point)` and `start(event, point)` place an event before
  it starts, and `position(instance, point)` moves one `start()` started. `position(instance)`
  reads the position back from FMOD. An event authored as 2D ignores its position. The position is
  in the same world coordinates as a clip's, and the game converts nothing. FMOD runs
  right-handed, and for a listener of the other hand the backend reflects the world through
  x = 0 on its way to FMOD and back. FMOD's distance unit is a metre by default, so a world unit
  is heard as a metre.
- **Events hear from the clip engine's listener.** `audio::Player::attach(events)` hands the
  backend the listener at once, and again at every `listener()` after. A game that plays events
  without a clip engine calls `events->listener(listener)` itself, with a listener
  `audio::orthonormal()` returned. The null backend refuses every position and listener.
- `name()` is `"fmod"` or `"null"`, for the log.

Background: [ADR-0021](../../adr/0021-audio-use-sdl3-mixer.md),
[ADR-0079](../../adr/0079-assets-loaders-are-registered.md),
[ADR-0084](../../adr/0084-audio-events-and-parameters-behind-an-interface.md),
[ADR-0112](../../adr/0112-audio-a-voice-is-placed-in-world-coordinates.md)
