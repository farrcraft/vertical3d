# Contributing

## Where engine code is written

**Change engine code in this repository, on a feature branch, and nowhere else.**

Other projects use this tree as a git submodule, usually at `vendor/vertical3d`. A submodule
is a full clone, so nothing stops you committing and pushing from inside it. Don't. It goes
wrong in three ways:

- **The change is hard to see.** In the consuming project, `git status` shows only that the
  submodule pointer moved. The diff sits in a nested repository nobody opens.
- **The pointer can name a commit nobody else has.** A commit made inside the submodule exists
  only in that checkout. Recording it in the consuming project gives a repository no other
  clone can check out.
- **The change can't be tested there.** `V3D_BUILD_APPS` and `V3D_BUILD_TESTS` are off when
  this tree is reached through `add_subdirectory`. A consumer's build compiles the api
  libraries it selected and nothing else: no apps and no test suites. A change that builds
  cleanly there has had none of this tree's own tests run against it.

## The loop

```
git switch main && git pull
git switch -c fix/short-description
```

Make the change, then build and test it here, where the apps and the suites are on by default:

```
scripts\build.cmd
ctest --test-dir out\build\x64-Debug --output-on-failure
```

[docs/contributing/Linting.md](docs/contributing/Linting.md) lists the four checks the tree is
expected to pass cleanly.
Then push and open a pull request.

## If you found it from a consumer

This is common, and useful: a consuming project uses the api the way a real application does,
so it finds problems the tree's own tests miss.

Edit inside `vendor/vertical3d` as much as you like while you work out what is wrong. Just
don't commit there.

When you know the fix, move it across instead of retyping it. Two local clones can fetch from
each other directly:

```
git fetch <path-to-consumer>/vendor/vertical3d main
git switch -c fix/short-description origin/main
git cherry-pick FETCH_HEAD
```

Then reset the consumer's submodule so it stops carrying an unpublished commit:

```
git -C <path-to-consumer>/vendor/vertical3d switch main
git -C <path-to-consumer>/vendor/vertical3d reset --hard origin/main
```

Finish the change here, with the apps and suites built.

## Testing a branch against a consumer

Checking out a branch in a consumer to test it is encouraged. Check the branch out in the
submodule and **leave the pointer change uncommitted**:

```
git -C <path-to-consumer>/vendor/vertical3d fetch origin fix/short-description
git -C <path-to-consumer>/vendor/vertical3d checkout FETCH_HEAD
```

The consumer shows `vendor/vertical3d` as modified until the branch merges. That is expected.
A consumer should only ever commit a submodule commit that is reachable from this repository's
`origin/main`. A `pre-commit` hook in the consumer can enforce this.

## After it merges

The consumer moves its submodule pointer to the merged commit and commits that like any other
change. The submodule commit is the only version pin: there are no releases and no
compatibility checks. [docs/api/UsingTheApi.md](docs/api/UsingTheApi.md#keeping-up-with-the-tree)
covers keeping up with the tree.
