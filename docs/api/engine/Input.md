# Events and input

How SDL events reach an app, how keys become commands, and how to read the keyboard and mouse
directly.

## Events and input

Input reaches an app in four ways. Use whichever fits:

- **Raw SDL events**, through `onEvent()`. Use this to host a ui toolkit you did not write.
- **Source events**, on the dispatcher's `sink<event::Source>`. Use this to capture a key.
- **Commands**, on the dispatcher's `sink<event::Event>`, made from source events by the
  binding document. Use this when the action should be named in config rather than in code.
- **Polling**, through `keys()`, `mouse()` and `held()`. Read `held()` from `simulate()`, and
  the edges from `tick()` or `render()`.

### Event routing

Each polled SDL event is offered to three places, in this order:

1. **The app's `onEvent(const SDL_Event&)`.** Returning true consumes the event, and the input
   devices never see it.
2. **The input devices**, which update the key and mouse state and publish source events. A
   key or mouse event they handle stops here.
3. **The engine's own handling** of quit, window close, resize and focus. This runs whatever
   `onEvent()` returned.

On a resize the engine updates the window's size and publishes
`event::kind::WindowResize`. On a focus change it publishes `event::kind::WindowFocus`. A key
released while the window is unfocused never arrives, so an app that tracks held keys itself
should drop them when focus is lost.

Rules for `onEvent()`:

- **Returning true hides the event from the bindings.** An app that returns true for every
  event disables its own binding document, and nothing reports it. Return true only for events
  the app's own ui actually used. A click on a button the app drew should not also fire the
  command bound to that click.
- **Keep it short.** It runs inside the poll loop, so a slow handler shows up as input latency.
- The default returns false. An app that does not host a ui toolkit does not override it.

Background: [ADR-0043](../../adr/0043-input-apps-see-raw-events-before-bindings.md)

### Keys and commands

A key or mouse button going down or up is a **source event**, `v3d::event::Source`. A named
action that a binding makes from it is a **command**, `v3d::event::Event`. They are published on
two separate sinks of the dispatcher:

| Sink | Carries | Listen here to |
|---|---|---|
| `sink<event::Source>` | every key and button edge | capture a key, or react to a raw key |
| `sink<event::Event>` | commands only | act on named actions |

A device also publishes typed events on the dispatcher for every input:
`event::kind::KeyDown`, `KeyUp`, `MouseButton`, `MouseMotion`, `MouseWheel` and `TextInput`. An
app can listen to these directly and use no binding document at all. voxel and the editor do
this for mouse motion and resize.

`event::publish()` sends a source event in two stages:

1. Every listener on `sink<event::Source>` hears it.
2. Unless one of them called `consume()` on it, the event engine sends the commands it is bound
   to on `sink<event::Event>`.

Every listener therefore hears the key before any listener hears its command, whatever order
they connected in. A key capture, such as a "press a key to rebind" menu item, listens on
`sink<event::Source>` and calls `consume()` on the key it takes. That key then makes no command.
pong's `handleSource` is an example.

**Send a source event only through `event::publish()`.** A source triggered directly on the
dispatcher reaches its listeners but is never mapped to a command.

A command's identity is `context::name`, for example `pong::leftPaddleUp`. Its `state()` is
`Pressed` or `Released`, copied from the source that triggered it, so one binding can serve both
edges. Its `data()` is the binding's `param`, if it has one.

A held key repeats at the platform's repeat rate. Each repeat is another `Pressed`, with
`repeat()` true on the source and on the command made from it. A command that acts for as long
as its key is held, such as moving a tetris piece, takes repeats. A command that toggles
something, such as a menu or an overlay, ignores a command whose `repeat()` is true. Otherwise
holding its key flicks it on and off.

Background: [ADR-0081](../../adr/0081-input-key-events-and-commands-are-separate.md)

### Bindings

The binding document is the config document of type `binding`, conventionally
`data/mappings.json`. It is a list of mappings from a source to a destination command:

```json
{
  "mappings": [
    {
      "source": { "context": "keyboard", "name": "w", "state": "pressed" },
      "destination": { "context": "pong", "name": "leftPaddleUp", "param": 1 }
    }
  ]
}
```

- `source.context` is `keyboard` or `mouse`. `source.name` is a key name from the table in
  [api/input/Keyboard.cpp](../../../api/input/Keyboard.cpp), such as `w`, `escape` or
  `arrow_up`, or a mouse button: `left`, `middle`, `right`, `x1` or `x2`.
- `source.state` is optional. `pressed` or `down` binds the press only, `released` or `up` the
  release only, and anything else, including no state, binds both.
- `destination.context` and `destination.name` name the command. `destination.param` is
  optional and becomes the command's data. It may be a whole number, a boolean or a string.
- A name no device can send is logged and bound anyway. It never fires.
- Every `name`, `context` and `state` is a string. A malformed document, including one that
  gives any of them another type, is logged, and startup fails.

`Engine::rebind(command, key)` points a command at a different key while the app runs, for a
rebinding screen. The command keeps the context and the edge the document gave it. A rebinding
lasts only as long as the process. To keep it, store it in [Settings](Files.md#settings-and-the-players-files)
and apply it again at startup, as pong's `applyStoredBindings()` does.

### Polling the keyboard and mouse

`Engine::keys()` returns an `input::KeyState` and `Engine::mouse()` an `input::MouseState`.
Either is null if the app did not ask for that device's feature.

| Call | Answers |
|---|---|
| `held(name)` | Is it down now? |
| `pressed(name)` | Did it go down during this frame's events? |
| `released(name)` | Did it come up during this frame's events? |
| `mouse()->position()` | Where the cursor is, in window pixels. |
| `mouse()->wheel()` | How many notches the wheel turned this frame. Positive is away from the user. Several notches in one frame add up. |

Keys and buttons are named, using the same names as the binding document.

The loop clears the edges after `render()`, so `tick()` and `render()` see the current frame's
edges. Read edges there and not in `simulate()`. A frame runs as many simulation steps as time
has passed for, which can be none or several, so an edge read in `simulate()` can be missed or
seen twice. A key pressed and released within one frame answers both `pressed()` and
`released()`, and is never `held()`. Polling SDL directly cannot tell you that.

`Engine::held("context::name")` is true while any key bound to that command is down. Use it for
movement or a camera pan read on the fixed step. It reads the keyboard state rather than
counting command edges, so it works for a binding that fires on press only, and it follows a
`rebind()`. Two limits:

- Only keys count. A mouse button bound to a command never makes it held.
- A command is its name and context, without its param. Directions that must be held
  independently need separate commands, not one command with different params.

### Driving an isometric camera

`engine::IsometricController` moves a `type::camera::Isometric` the app owns, from commands:

```cpp
// in start()
controller_ = std::make_unique<v3d::engine::IsometricController>(&orbit_, *this);

// in simulate(step)
controller_->simulate(step);

// in release()
controller_.reset();
```

- **Bind the keys in the binding document**, to `camera::rotate_left`, `camera::rotate_right`,
  `camera::pan_up`, `camera::pan_down`, `camera::pan_left`, `camera::pan_right`,
  `camera::zoom_in` and `camera::zoom_out`. Other names can be given in
  `IsometricController::Commands`.
- **A rotate command turns one step per press.** A repeat of a held key is ignored, so holding
  the key does not spin the camera. Left turns clockwise seen from above, and right
  counterclockwise.
- **Pan and zoom move while their command is held**, read with `held()` from `simulate()`. The
  speeds are in `IsometricController::Speeds`: world units a second for a pan, and orthographic
  half height a second for a zoom. A pan follows the view's own axes, so up stays up on screen
  after a rotate.
- Destroying the controller disconnects it from the dispatcher.

### Mouse look

Call `window()->relativeMouse(true)`. The pointer is hidden and held inside the window, and
`event::kind::MouseMotion::motion()` reports how far the mouse moved, even at the edge of the
screen. The platform releases the mouse while the window is unfocused and takes it back when
focus returns. Turn the mode off only for the app's own reasons, such as a menu that needs a
pointer. voxel does this while its menu is open.
