# scripts

The tree's own tooling.

- `build.cmd` and `test.cmd` enter the MSVC developer environment and run ninja or ctest on
  `out/build/x64-Debug`. [docs/contributing/Build.md](../docs/contributing/Build.md) covers them.
- Each `.ts` file at this level but `docs.ts` and `pictures.ts` is a review gate, run as
  `node scripts/<gate>.ts`. `lib/` holds what the gates share, and `tests/` holds their tests.
  [docs/contributing/Gates.md](../docs/contributing/Gates.md) lists the gates and what each
  checks.
- `docs.ts` builds the documentation site into `out/docs/`, with the configuration and the
  preprocessor in `docs/`.
- `pictures.ts` renders the pictures the documents show, from scenes under `docs/`, with a built
  moya.
- `portable.cmd` and `portable/` run the Linux Clang build in a container, locally as in CI.
  [docs/contributing/Build.md](../docs/contributing/Build.md#building-off-msvc) covers it.

A gate runs with Node alone. `npm ci` installs only the type checker.
