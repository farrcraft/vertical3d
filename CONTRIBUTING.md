# Contributing

## Where engine code is written

**In this repository, on a feature branch, and nowhere else.**

That is worth stating plainly because this tree is consumed as a submodule rather than as a
package, and a submodule is a complete clone: a consumer can branch, commit and push from
inside `vendor/vertical3d` without anything stopping them. It works, and it goes wrong in
three ways.

**The work becomes invisible.** In the consumer, `git status` shows one line — a changed
submodule pointer. The diff is in a nested repository nobody has reason to open, so a change
can sit in a working tree for days belonging to neither project.

**The pointer can name a commit that does not exist.** A commit made on the submodule's `main`
is local to that checkout. Recording its sha in the consumer produces a repository that no
other clone can resolve, and a rebase in the submodule orphans even the local copy.

**The change cannot be verified there.** This is the one that decides it. `V3D_BUILD_APPS` and
`V3D_BUILD_TESTS` default to `PROJECT_IS_TOP_LEVEL`, which is false when this tree is reached
through `add_subdirectory`. A consumer's build configures the api libraries it selected and
nothing else — not the nine applications, not one Boost.Test suite. A change that builds clean
in a consumer has had none of the tree's own coverage run against it. Anything touching
`cmake/`, a shared helper, or an api that more than one app calls will be under-tested by
exactly the parts most likely to catch it.

So the rule is not about tidiness. A submodule is a place where this tree's code cannot be
tested.

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

[docs/Linting.md](docs/Linting.md) has the four gates the tree is expected to be clean at.
Then push and open a pull request.

## If you found it from a consumer

Normal, and the best case — a consumer exercises the api the way an application actually uses
it, which is why cozy's first screen of controls found two `api/ui` defects the tree's own
tests could not see.

Edit inside `vendor/vertical3d` as much as you like to work out *what is wrong*. Diagnosis
belongs where the symptom is. What must not happen is a commit there.

When you know the fix, move it rather than re-typing it — two local clones can talk directly:

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

Supported and encouraged — it is the thing a consumer is better at. Check the branch out in
the submodule and **leave the pointer change uncommitted**:

```
git -C <path-to-consumer>/vendor/vertical3d fetch origin fix/short-description
git -C <path-to-consumer>/vendor/vertical3d checkout FETCH_HEAD
```

The consumer will show `vendor/vertical3d` as modified. That is correct and it stays that way
until the branch merges. A consumer should only ever commit a sha reachable from this
repository's `origin/main`; cozy enforces that with a `pre-commit` hook, and any consumer can
do the same.

## After it merges

The consumer moves its pointer to the merged commit and commits that like any other change.
The submodule sha is the only version pin that exists — there is no release and no
compatibility check, so see [docs/NewProject.md](docs/NewProject.md#keeping-up-with-the-tree).
