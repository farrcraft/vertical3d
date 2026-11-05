# ADR-0174: Testing: the hand-written parsers are fuzzed

**Status**: accepted
**Date**: 2026-11-04
**Tags**: testing, ci, offline
**Documented in**: [Testing.md](../../contributing/Testing.md#fuzz-targets)

## Context

The tree parses several formats itself: RIB, the shading language, TGA and BMP. Their input
comes from files a user writes or downloads, so a malformed file is ordinary input, not a
fault. A unit test covers the malformed cases its author thought of. Generated input reaches
the ones nobody thought of, and the sanitizers turn a quiet misread into a failure.

## Decision

Each hand-written parser has a fuzz target that libFuzzer can drive, and every build builds it.
ctest runs each target over its library's fixtures once, so it runs wherever the tests run. The
Linux CI job builds the targets with libFuzzer and both sanitizers, and fuzzes each for a fixed
time on every run.

## Alternatives

### OSS-Fuzz
- **For**: fuzzing that never stops, on Google's machines, with reports filed automatically.
- **Against**: it accepts only projects under an open source licence that are widely used,
  and runs outside the repository's CI.
- **Rejected because**: the repository has no licence, so it does not qualify.

### ClusterFuzzLite in CI
- **For**: keeps the corpus between runs, and can fuzz for longer on a schedule.
- **Against**: it builds in an image of its own on the OSS-Fuzz toolchain, beside the image
  the Linux job already builds in, so the libraries would have two build definitions.
- **Rejected because**: the job's own image already has Clang and the packages, and the same
  targets run under ClusterFuzzLite unchanged if longer runs are wanted.

### AFL++
- **For**: stronger mutation strategies, and it can fuzz a program with no entry point written
  for it.
- **Against**: its own compiler wrappers, built and installed beside the job's Clang.
- **Rejected because**: libFuzzer ships with the Clang the job already has, and a libFuzzer
  target also runs under AFL++ if it is ever wanted.

### More malformed-input cases in the unit tests
- **For**: no new tooling, and every case is written down.
- **Against**: each case is one its author already thought of, so the suites never find the
  input nobody foresaw.
- **Rejected because**: finding unforeseen input is the purpose, and only generated input does.

## Consequences

- **Gains**: each parser is checked against input nobody wrote a test for, under both
  sanitizers. A target cannot rot, because every build builds it and every test run runs it.
- **Costs**: the CI job runs longer by the fuzzing time. A short run on each pull request
  reaches less than a long one would, and CI starts from the fixtures every time, because it
  keeps no corpus. A new parser needs its own target written by hand.
- **Revisit when**: a long-running fuzzer finds what the short runs miss, or the repository
  takes an open source licence and OSS-Fuzz becomes open to it.
