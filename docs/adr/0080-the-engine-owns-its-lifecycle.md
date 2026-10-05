# ADR-0080: App Lifecycle — The Engine Owns Its Order, And An App Supplies Hooks

**Date**: 2026-10-05
**Status**: accepted
**Deciders**: Joshua Farr

## Context

`engine::Engine::initialize(int)` and `shutdown()` were public, and `shutdown()` virtual. Every
app wrapped both: its own `bool initialize()` called the base with a feature mask and then did its
work, and its own `shutdown()` tore its renderer down and then called the base. Six engines carried
that shape, and two rules CLAUDE.md lists as its costliest were kept only by copying it correctly:
a renderer goes before the window, and a quit handler calls `quit()` rather than `shutdown()`. Five
handlers carried a comment saying so. The app's `initialize()` hid the base's rather than
overriding it, and three apps declared `render()` and `shutdown()` without `override`. The review
is [E1](../audits/ApiDesignReview.md).

## Decision

**The base owns the order, and an app supplies what runs inside it.** `initialize()` is not
virtual and takes nothing: it sets up the `features()` the app asks for — all four by default —
and then calls the app's `start()`. `shutdown()` is not virtual and is private to `run<T>()`: it
calls the app's `release()` once, then destroys the window and quits SDL. An app's handler cannot
reach `shutdown()` at all, so `quit()` is the only thing left to call. Features are a
`type::Flags<Feature>` rather than an `int`.

## Alternatives Considered

### Alternative 1: Teardown in destructors
- **Pros**: no `shutdown()` at all. C++ destroys a derived class's members before its base's, so
  an app's renderer would go before the engine's window by construction, and `run<T>()` would only
  let the engine go out of scope.
- **Cons**: every renderer's destructor has to do what its `shutdown()` does now, waiting for the
  device among it, and a teardown that fails has no way to say so. The order would hold by a
  property of the language a reader has to know, rather than by a call they can read.
- **Why not**: it trades a rule nobody could break for one nobody can see.

### Alternative 2: Keep the virtual pair and refuse `shutdown()` while the loop runs
- **Pros**: the smallest change, and it stops the quit mistake at run time.
- **Cons**: the order of teardown is still each app's to get right, and the mistake is still
  written, only refused.
- **Why not**: it catches one of the two rules and neither of them at compile time.

### Alternative 3: Hooks, with the base owning the order — **chosen**
- **Pros**: the two rules are structural: release runs before the window goes because the base
  calls it there, and a handler cannot call `shutdown()` because it cannot see it. An app's
  lifecycle is only its own work. The default feature set removes the mask five apps repeated.
- **Cons**: see below.

## Consequences

### Positive
- No app tears down the window, so no app can do it in the wrong order or from a handler.
- `EngineTest` asserts at compile time that `shutdown()` is out of reach and `quit()` is not.
- An app that wants fewer features says so once, in `features()`.

### Negative
- Teardown is two places, `release()` and `~Engine`. The destructor covers a run that never got
  to `shutdown()` — a test, a failed start — and can only release the engine's own window, since
  an app's members are gone by the time a base destructor runs.
- `run<T>()` is a friend of `Engine`. Anything else that wants to drive an engine to completion
  goes through it.

### Risks
- An app that holds something presenting to the window outside `release()` — a second renderer
  built later, say — still has to put it there. The escape hatch is that `release()` is the one
  place to look.
