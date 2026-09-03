# ADR-0017: Command Dispatch — A Command Is A Name In A Context, And The Directory Is The Editor's

**Date**: 2026-09-02
**Status**: accepted
**Deciders**: Joshua Farr

## Context

The editor answers to twenty-odd things a user can ask for: make a cube, change the select
mask, choose a manipulator, hide the grid, undo, quit. Every one of them arrived as a mapped
`event::Event` and was answered by `Controller::handleEvent`, which was a chain of
comparisons — first on the event's context, then on its name — a hundred and thirty lines
long and growing by a branch per command.

The menus are what makes that a problem rather than an untidiness. `rigel/docs/xml/gui.xml`
is the editor's UI definition and names 51 commands, each as a string like
`create::poly::cube` on a `<menuitem>`, a `<button>` or a `<bind>`. Three different things
name the same command, and rigel's answer was `CommandDirectory`: a string to `Command` map
that all three looked into. That directory went to `v3dlibs/command` and from there to
`api/event`, where it became a mapping from an input event to a named destination event —
and the name-to-behaviour half was not carried across. So the editor could turn a keypress
into a command name and then had nowhere to look it up.

`api/ui` already produces the other kind of invocation. A `ui::component::MenuItem` holds a
`v3d::event::Event` and `Menu::activate()` triggers it on the same dispatcher a binding's
destination goes to, marked `Type::Destination` exactly as `engine::Engine`'s binding loader
marks one. The two invocations are already the same object; nothing was reading them the
same way.

## Decision

**A command is identified by its context and name together — `"context::name"`, which is what
`event::Event::str()` returns.** `v3d::editor::CommandDirectory` maps that string to a
handler, `Controller` registers one per command at startup, and `handleEvent` is a lookup and
nothing else. A key binding and a menu item that carry the same event reach the same handler,
because the identity is all either of them contributes.

Four things follow.

- **The command names are gui.xml's.** `create::poly::cube`, `select::mask::object`,
  `transform::translate`, `view::show::grid`, `view::camera::zoom`. `data/mappings.json` was
  rewritten onto them, so the key bindings and the menu translation that has not happened yet
  name the same commands. Two deviate: `ui::quit`, because gui.xml gives quit no context and
  `ui` is where every app in this repository puts its application level commands; and
  `edit::undo`/`edit::redo`, which gui.xml does not have at all.
- **Only a destination event is a command.** The dispatcher carries both halves of a mapping,
  so the raw keypress reaches the same sink the mapped command does. `event::Engine` guards
  itself with `type() != Type::Source`; the editor guards itself with the converse. Without
  it every keypress is looked up under `keyboard::3` and found missing.
- **An unregistered name is reported, not ignored.** `invoke()` says whether a handler
  existed and the controller logs the ones that did not. Rigel's equivalent branch was
  "Unhandled Command!" for 24 of its 51 commands, and being able to see which is what makes a
  menu translated from a file into a checklist against `names()`.
- **A duplicate registration is refused rather than overwriting.** Two handlers for one
  command is a bug in which one of them silently never runs; `add()` returns false and keeps
  the first.

**`Tool` stays in the editor**, which is the other half of the item this record closes. A
tool is a command that keeps receiving motion and button events for as long as it is the
active one — `activate`, `deactivate`, `motion`, `button`. Rigel put that in `libv3dcommand`
beside its `Command`, and the natural mirror would be `api/event` beside the dispatcher. No
game in the repository has a tool: input reaches them as discrete named events and they need
nothing that holds a gesture open across them. The interface stays where its only consumer
is, on the reasoning of [ADR-0016](0016-undo-records-what-has-already-happened.md) — one
consumer is not a library.

## Alternatives Considered

### Alternative 1: A directory keyed on the event's identity, handlers registered by the controller — **chosen**
- **Pros**: A binding, a menu item and a toolbar button are already the same `event::Event`,
  so all three dispatch through one path with nothing added for the second and third. The
  registration is a list of what the editor can do, in one place, checkable against a menu
  file. A handler closes over whichever tool it drives, so the controller stops being the
  thing that knows how each subsystem spells its own modes. And an unknown command is
  answerable rather than silently dropped.
- **Cons**: A `std::function` per command and a string built per event, where the chain
  compared string views. Registration happens at startup and a name typed wrong in it is
  found by running rather than by compiling.
- **Why not**: n/a — chosen.

### Alternative 2: Keep the chain, and give the menu its own dispatch
- **Pros**: Nothing to write until the menus land, and no indirection between an event and
  what it does.
- **Cons**: Two paths to the same behaviour that have to agree, which is exactly the drift
  the mapping layer was built to remove. A menu item would name a command string the chain
  does not use, so the translation would be from gui.xml's names to the chain's — a second
  table nobody would keep current.
- **Why not**: It makes the menu translation harder than the thing it is meant to enable.

### Alternative 3: Put the directory in `api/event` beside the dispatcher
- **Pros**: That is where rigel's lived, and where the mapping half of it already is. Every
  app would get named commands rather than a hand written event handler.
- **Cons**: The four games each answer a handful of events and do it in a switch that costs
  them nothing. The directory's value is entirely in there being several ways to invoke the
  same command, which only an editor has.
- **Why not**: Same reasoning as the command stack in ADR-0016. It moves when a second app
  wants it.

### Alternative 4: Register a handler per context, and let it split on the name
- **Pros**: Closer to what was there — six handlers instead of twenty-two registrations — and
  a context is a real grouping.
- **Cons**: `create::poly::cube` and `create::ortho::camera` share a context and nothing else,
  so the handler is the same chain one level down. A menu item still names a whole command,
  so the directory would answer half a lookup and hand the rest back.
- **Why not**: It keeps the branch per command and only moves it.

## Consequences

### Positive
- `Controller::handleEvent` is three lines, and adding a command is one registration next to
  the others rather than a branch in the middle of a chain.
- The command names in `data/mappings.json` are the ones gui.xml uses, so translating its
  menus is a matter of writing the menu tree — the commands the items name already resolve.
- Verified against a run on 2026-09-02: a cube and a cylinder created, all four select masks,
  all four transform modes, the three visibility toggles, undo, redo, undo again and quit,
  each firing exactly once with nothing unhandled and the validation layer silent.
- Nine cases cover the directory — identity, context separation, the unknown name, the
  refused duplicate, the state passed through, the press-only form and the name listing —
  none of which needs a window or a device.
- The source event guard fixes a defect the chain was hiding: every keypress was reaching the
  editor's handler twice, once as itself and once as what it mapped to, and the chain dropped
  the first silently because the `keyboard` context matched none of its six.

### Negative
- The registration is checked at startup rather than at compile time. A name misspelled in
  `registerCommands` and a name misspelled in `mappings.json` both surface as a logged
  unhandled command, which is a run to find rather than a build.
- Every handler is a lambda closing over `this`, so the directory outlives nothing and the
  controller cannot be destroyed while it holds one. That is true of the controller's other
  members too, but it is now true of a container that could otherwise be moved.
- 32 of gui.xml's 51 commands still have no handler — the timeline, the importers, render
  settings, cull and shade modes, the seven view cameras, camera creation and grouping. The
  22 registrations cover 19 of them, plus `view::drag` and the two history commands, which
  gui.xml does not name. A menu translated whole would
  list them and they would log unhandled, which is honest but is not a working menu.

### Risks
- `view::show::camera` and `view::show::light` are deliberately not registered even though
  `ViewPort` has the flags: nothing draws a camera or a light, so a toggle would be a
  menu item that appears to work and does nothing. They go in when there is something to hide.
- Nothing yet checks a menu file against `names()`. The listing exists for it; the check
  arrives with the menus.
