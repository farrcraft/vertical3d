# ADR-0033: API Selection — A Consumer Names The Libraries It Wants, And A Manifest Expands The Closure

**Date**: 2026-09-06
**Status**: accepted
**Deciders**: Joshua Farr

## Context

[ADR-0027](0027-the-api-is-consumed-as-source.md) made the root `add_subdirectory`-able, and every
api library carries its own include root, its own `v3d::` alias and its own dependencies. What it
did not make selectable is *how much* of the api a consumer takes: `add_subdirectory("api")` added
all sixteen libraries unconditionally, and `cmake/v3dDependencies.cmake` ran every `find_package`
before any of them.

So an application that wants `v3d::image` to read a PNG builds `v3dlib_render` too, and its
configure fails without the Vulkan SDK, without SDL3 and without the mixer — none of which it
names, links or compiles a line against. The dependency surface a consumer has to install is the
union of the whole api rather than the closure of what it asked for.

`v3dlib_render_offline` is the sharpest case. Its own CMakeLists says it "names neither Vulkan nor
SDL", which is what lets talyn and moya's suites run in CI where the realtime stack cannot — and it
was added from inside `api/render/CMakeLists.txt`, so nothing could take it without the realtime
stack anyway.

The build cost is the smaller half. The larger half is that nothing in this repository can observe
the difference: every library is always built here, so a library that links something it never
declared works, and only a consumer taking a narrow slice ever finds out.

## Decision

**A consumer names the libraries it links in `V3D_LIBRARIES`, and the tree expands the closure.**
It defaults to `all`, so a build of this repository is unchanged. `cmake/v3dApiLibraries.cmake`
holds the manifest: for each library, the api libraries it requires and the third-party packages it
names. The closure of the requested set decides both which subdirectories are added and which
packages are looked for.

**A package is found only if something in the closure names it.** `find_package` moves out of
straight-line code in `cmake/v3dDependencies.cmake` and into `v3d_find_packages`, called once from
the root with the computed set. It is a macro rather than a function on purpose: an imported target
is created in the scope `find_package` runs in, a function scope disappears when it returns, and
`find_package(... GLOBAL)` needs CMake 3.24 where this tree requires 3.21.

**Boost is the exception and stays unconditional.** `v3d_add_api_library` links `Boost::headers`
into every library, so there is no selection that avoids it.

**`api/render/offline` is added by `api/CMakeLists.txt`, not by `api/render`.** The directory does
not move; only the `add_subdirectory` does, so the offline library can be taken without the
realtime one. It sets `/utf-8` itself, which it had been inheriting from `api/render`'s directory
flags.

**The manifest is verified against the real link graph at configure time.**
`v3d_api_verify_manifest` runs once every selected library has been added, reads each target's
`LINK_LIBRARIES` and `INTERFACE_LINK_LIBRARIES`, and fails the configure on a difference in either
direction. This is what makes the duplication safe: the manifest is a second statement of what each
library's `target_link_libraries` already says, and a second statement that is not checked is a
second statement that will eventually be wrong.

**Narrowing the selection with the apps or the tests on is an error, not a silent widening.** The
apps name every library between them and each tests directory belongs to one library, so either
means all of them.

## Alternatives Considered

### Alternative 1: A manifest, a closure, and a verification — **chosen**
- **Pros**: The consumer states what it links and nothing else; the closure is the tree's problem,
  which is the half it is qualified to answer. The verification turns the duplicated fact into a
  checked one. Most of it is what an installed package's generated config needs anyway —
  `find_dependency` per library *is* this manifest — so it is
  [ADR-0027](0027-the-api-is-consumed-as-source.md) Alternative 2's work arriving early and being
  exercised.
- **Cons**: A new CMake module, and a dependency edge is now recorded in two places. Adding a
  package to a library means editing its CMakeLists and the manifest, and forgetting the second is
  a failed configure rather than a quiet success.
- **Why not**: n/a — chosen.

### Alternative 2: Drop `REQUIRED` and skip what is not found
- **Pros**: Perhaps fifteen lines. No manifest, no closure, no new knob, nothing to keep in step.
- **Cons**: A consumer with a broken Vulkan install gets no `v3d::render` and, because engine
  requires it, no `v3d::engine` either — reported as an unknown target in *their* CMakeLists,
  naming neither Vulkan nor this decision. It also cannot express intent: a consumer that wants only
  `v3d::image` still builds everything its machine happens to be able to build.
- **Why not**: It answers "what can this machine build" when the question is "what does this
  application need". Every failure it produces is reported far from its cause.

### Alternative 3: A `V3D_BUILD_<LIB>` option per library
- **Pros**: The conventional CMake shape, greppable, and each option documents itself in
  `cmake-gui`.
- **Cons**: Seventeen booleans, and the closure becomes the consumer's problem — asking for
  `engine` without `render` is a configure error about an unknown target that they have to work
  out. The closure is exactly the part of this that the tree knows and the consumer does not.
- **Why not**: It moves the hard half of the job to the party with the least information.

### Alternative 4: `find_package` inside each library's own CMakeLists, and no selection
- **Pros**: The dependency fact stays in one place, which is the whole objection to the manifest.
  No closure machinery at all.
- **Cons**: It does not fix anything. `add_subdirectory("api")` still adds every library, so
  `api/render` still runs `find_package(Vulkan REQUIRED)` and the consumer still needs the SDK.
  Worse, an imported target created inside `api/<lib>/` is not visible to a sibling app directory at
  the root, so the apps would stop linking.
- **Why not**: It addresses the half of the problem that was never the expensive half.

### Alternative 5: An installed package with a generated config
- **Pros**: The real answer to "take part of this api", with `find_dependency` per component.
- **Cons**: [ADR-0027](0027-the-api-is-consumed-as-source.md) Alternative 2 rejected it and nothing
  has changed: it needs install rules, both configurations installed with generator expressions, and
  the consumer still has to match the vcpkg baseline exactly.
- **Why not**: Same reasoning as ADR-0027. This decision is more of its groundwork, not a
  replacement for it.

## Consequences

### Positive
- A consumer's installed dependency set is the closure of what it links. `V3D_LIBRARIES=image;log`
  configures and builds with no Vulkan, no SDL3, no Freetype and no EnTT anywhere in its cache.
- `v3d::render_offline` is takeable on its own, so an offline renderer in another repository needs
  no graphics driver, no SDK and no window system.
- The verification is a real net rather than a gesture: it caught a mistake in the manifest it was
  written alongside, on its first run.
- `examples/starter` names the three libraries it links, so CI exercises a narrow closure on every
  push. That is the only place in the tree where an api library failing to declare an edge is
  visible at all.

### Negative
- Every dependency edge is stated twice, in the library's `target_link_libraries` and in the
  manifest. The verification makes disagreement loud rather than impossible, and a new package means
  editing two files.
- `cgltf` contributes an include directory rather than an imported target, so nothing can confirm
  `api/asset` still uses it. Its manifest entry is taken on trust.
- The root is four cmake files instead of three.

### Risks
- The manifest names packages, not versions or components. A library needing a *new* Boost component
  still edits the one unconditional `find_package(Boost)`, and nothing points at it.
- A library reached only through a generator expression in a link line would not be recognised by
  the verification. Nothing in the api does this today, and a library that started to would be
  quietly unverified rather than loudly wrong.
