# The mouse

How a press and the cursor's position reach a component, and when they reach the app instead.
What each control sends when clicked is in [README.md](README.md#commands).

## The mouse

`v3d::ui::input::Cursor` routes the mouse over a `ui::Engine`. The app passes it points, and
each call returns whether the ui used the point. The app acts on a point only when the ui
did not.

```cpp
uiCursor_ = boost::make_shared<v3d::ui::input::Cursor>(vgui_, dispatcher(), text->measure());

bool taken = uiCursor_->press(cursor);   // also motion(point) and release(point)
if (!taken) {
    // the press is the app's: pick in the scene, drive a camera ...
}
```

- `motion(point)` moves the hover highlight, and follows a held press.
- `press(point)` offers the point to the ui and sends a command if it lands on a control.
- `release(point)` ends a held press. It sends nothing.

The `Measure` argument is optional. It places the caret when a `TextBox` is clicked. Pass the
same `Measure` the renderer drawing the ui was given, so the two agree on where each
character is. A cursor given none still routes every press, but a click in a text box leaves
the caret where it was.

`Cursor` is not wrapped by the shell, because an app often interleaves the ui's press with
its own use of the mouse. The editor offers the ui a press first and drives its camera with
any press the ui did not take.

How a point is routed:

- Each visible container is offered the point, in the order the document lists them. The
  first container that takes it ends the search.
- Within a container, the menu bars are offered the point first, then the toolbars, then the
  component tree. This is the reverse of the draw order.
- In the tree, the topmost component under the point is found. A child is offered the point
  before its parent.
- A component that is not `pickable()` is passed over, and the search continues beneath it.
  A HUD of labels over a scene therefore leaves the scene clickable.
- A press that lands on nothing pickable is not consumed. It belongs to the app.
- A strip takes a press anywhere on it, including the gaps between its buttons. A strip
  that is hidden, disabled or marked `"pickable": false` is skipped, and the press falls to
  the tree under it.
- A press that lands is held until release. While it is held, every `motion()` goes to the
  held component. This is how a scrollbar thumb, a slider thumb and a text selection are
  dragged.

Hover:

- A button lights up under the cursor, on a strip or in the tree. Only the component a press
  would land on is lit, so unpickable labels never flicker as the cursor crosses them.
- Hover is a button state only. No other component has a hover look.

Everything is tested against the boxes left by the last draw. **Draw before routing input,**
or an app sees a dead ui for one frame. Nothing can be picked before the first frame is
drawn.

An `Immediate` layer has its own equivalent: `Immediate::capturing()`. See
[Immediate mode](ImmediateMode.md#immediate-mode).

Whether to offer the ui a cursor at all is the app's choice. A game that holds the mouse in
relative mode for mouselook has no meaningful cursor. `voxel` passes its immediate debug
panel a real `Input` only while its menu is up, and an empty `Input` otherwise.
