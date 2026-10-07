# ADR-0080: Apps: the engine owns startup and shutdown order

**Status**: accepted
**Date**: 2026-10-05
**Documented in**: [api/engine/Lifecycle.md](../api/engine/Lifecycle.md)

## Context

An app's startup sets up the engine's features before its own work, and its teardown must release
its renderer before the engine destroys the window. A quit handler must ask the loop to stop with
`quit()` rather than tear the engine down from inside an event. When `initialize()` and
`shutdown()` are public virtuals that every app wraps, both rules hold only if every app copies
the wrapper correctly. Nothing in the compiler checks either one.

## Decision

The engine owns the order, and an app supplies the work that runs inside it. `initialize()` sets
up the features the app asks for in `features()` and then calls the app's `start()`, and
`shutdown()` calls the app's `release()` once and then destroys the window and quits SDL. Neither
is virtual, and `shutdown()` is private to `run<T>()`, so an app's handler cannot reach it and
`quit()` is the only thing left to call.

## Alternatives

### Teardown in destructors
- **For**: no `shutdown()` at all. C++ destroys a derived class's members before its base's, so an
  app's renderer would go before the engine's window by construction.
- **Against**: every renderer's destructor has to do what its teardown does now, including waiting
  for the device, and a teardown that fails has no way to report it. The order would rest on a
  rule of the language a reader has to know, rather than on a call they can read.
- **Rejected because**: it trades a rule nobody can break for one nobody can see.

### Keep the virtual pair, and refuse `shutdown()` while the loop runs
- **For**: the smallest change, and it stops the quit mistake at run time.
- **Against**: teardown order is still each app's to get right, and the quit mistake can still be
  written, only refused when it runs.
- **Rejected because**: it catches one of the two rules, and neither at compile time.

## Consequences

- **Gains**:
  - No app tears down the window, so no app can do it in the wrong order or from a handler.
  - A test asserts at compile time that `shutdown()` is out of an app's reach and `quit()` is not.
  - An app's lifecycle code is only its own work, and an app that wants fewer features says so once,
    in `features()`.
- **Costs**:
  - Teardown is in two places, `release()` and the engine's destructor. The destructor covers a
    run that never reached `shutdown()`, such as a test or a failed start, and can release only the
    engine's own window, since the app's members are already gone.
  - `run<T>()` is a friend of `Engine`, so anything else that drives an engine to completion has to
    go through it.
  - Something that presents to the window and is built outside `start()`, such as a second
    renderer created later, still has to be released in `release()` by hand.
- **Revisit when**: an app needs to drive the engine some way other than `run<T>()`, or needs work
  between its own release and the window's destruction.
