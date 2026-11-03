// Check that the test cases a changeset adds fail without the changeset's code.
//
// A test that passes without the code it claims to test is a weak test. This script finds the
// Boost.Test cases a changeset adds and runs each one against the changeset's tests with the
// base version of everything else. Every new case should fail there, or fail to build.
//
//     node scripts/failsfirst.ts                    the branch, from its merge base with main.
//     node scripts/failsfirst.ts --base 13a9557     the commits after 13a9557.
//     node scripts/failsfirst.ts --commit 5ff63287  one commit, against its parent.
//     node scripts/failsfirst.ts --list             only print the new cases and their suites.
//     node scripts/failsfirst.ts --build out/build/ci   reuse another build's packages.
//
// Node runs this file directly, with its types stripped. It uses only Node's built-in modules
// and the gates' modules in scripts/lib.
//
// What it does:
//
// 1. Finds the new cases. A case is new when the head revision has it and the base revision
//    does not, in the same ctest suite under the same Boost.Test suite path. The macros read are
//    BOOST_AUTO_TEST_CASE, BOOST_FIXTURE_TEST_CASE, BOOST_DATA_TEST_CASE and their _F and
//    _TEMPLATE forms, inside any BOOST_AUTO_TEST_SUITE or BOOST_FIXTURE_TEST_SUITE. A case moved
//    between files of one suite is not new. A test file belongs to the suite whose
//    v3d_add_test(<suite> ...) list in a tests/CMakeLists.txt names it, and the suite's binary is
//    v3dtest_<suite>.
// 2. Adds a git worktree at the head revision, under the system temporary directory. The
//    checkout and the build in out/build are not touched. In the worktree, every file the
//    changeset changed outside a tests/ directory goes back to its base version, and a file it
//    added outside tests/ is removed. A file it deleted comes back at its base version, even
//    under tests/, so that a base CMakeLists.txt finds the test directories it adds. Everything
//    else under a tests/ directory keeps its head version: the test sources, their
//    CMakeLists.txt, and the fixtures under tests/data, tests/fixtures and tests/device/data.
//    A head v3d_add_test list can name a source outside tests/ that the change added, such as
//    an app source a suite compiles. That source is dropped from the list in the worktree. When
//    the configure reports a source missing, it is dropped from each list that names a file of
//    that name the worktree lacks; a list whose entry resolves to a file that exists keeps it.
//    A head tests/CMakeLists.txt can also call a CMake helper the change added outside tests/.
//    When the configure reports the command unknown, the call is removed from the worktree.
// 3. Configures the worktree into its own build directory and builds only the affected suites.
//    The configure reuses the packages the main build already installed: it points
//    VCPKG_INSTALLED_DIR at that install, turns VCPKG_MANIFEST_INSTALL off, and uses the
//    checkout's vcpkg toolchain file, so nothing is installed. The values come from the main
//    build's CMakeCache.txt, which is out/build/x64-Debug unless --build names another. A link
//    job pool the main build was configured with is configured here too, because the link
//    edges it serialises also copy dlls. vendor/libnoise is linked into the worktree with a
//    directory junction, because voxel links its prebuilt library. /WX is off, so that only an
//    error counts as a failure to build.
// 4. Runs each new case alone, as v3dtest_<suite>.exe --run_test=<path>, from the executable's
//    directory so that its fixtures resolve.
// 5. Prints one row per case and exits 1 when any case passed without a stated reason. The
//    worktree and its build are removed unless --keep is given.
//
// The results:
//
//     fails              the case ran and Boost.Test reported a failed check or test. This is
//                        the expected result.
//     fails to build     the suite does not compile or link without the change, usually because
//                        the case uses an API the change added. This counts as failing. When a
//                        test file fails to compile, or its object names a symbol the link
//                        cannot find, the file is dropped from the suite in the worktree and the
//                        rest are built and run. A suite whose directory the base does not
//                        configure, because the change added or moved it, fails to build too.
//     PASSES - weak      the case passes without the change. It does not test the change.
//     passes, stated     the case passes without the change, and its doc comment says why.
//     passes, covers the base
//                        the case passes without the change, and its doc comment names the
//                        commit in the base that brought the code it tests.
//     skipped           the binary found no Vulkan device, or render_window no window, and
//                        exited with 77.
//     not run            the binary did not find the case, timed out, could not start, or
//                        exited with an error and reported no failed check. A binary that
//                        cannot load a DLL is one of these, and so is a suite whose build
//                        made no executable.
//
// A case that guards against a defect only the change itself could cause passes on the base by
// design. Its doc comment says so in a line of this form, and the reason may wrap onto the lines
// that follow, up to the end of the paragraph:
//
//     Passes before the change: <why the defect cannot happen without the change>
//
// Such a case is reported as "passes, stated" with its reason, and does not make the run fail.
// --list prints the reason too. A reviewer reads the reason: it has to name what the change
// introduced that makes the defect possible.
//
// A case added for code that is already in the base also passes there, because the change does
// not touch that code. Its doc comment says so in a line of this form, which may wrap the same way:
//
//     Covers the base: <the commit that brought the code> <what the case was checked against>
//
// The first run of seven or more hex digits in it is the commit. When that commit exists and the
// base contains it, the case is reported as "passes, covers the base" and does not make the run
// fail. A line that names no such commit, including one the change itself made, counts for
// nothing, and the case is reported as weak. The harness cannot show that such a case would fail
// without the code it tests, so the line also says how that was checked: by breaking the code by
// hand and watching the case fail. A reviewer reads that part.
//
// The build environment is the one scripts\vcvars.cmd enters for scripts\build.cmd: vcvars64.bat
// under Visual Studio 18 or 2022 Community, or the file V3D_VCVARS names. A cold build of a suite
// compiles every library it links, which takes several minutes for the render suites.
//
// Limits:
//
// - Only committed revisions are compared. Uncommitted changes in the checkout are ignored.
// - A render_device case needs a Vulkan device, and a render_window case a window as well.
//   Without one the binary skips, and the case is reported as skipped, not as passing or
//   failing.
// - A case can fail without the change only because of a fixture the change added, such as a
//   file the old code cannot read. That counts as failing here, but it shows that the fixture
//   is new, not that the test checks the new code. Read such a case by hand.
// - A whole test file is the unit dropped after a compile or link error. A new case that would
//   have built is reported as failing to build when it shares a file with one that did not.
// - A link error that names no object of the suite, such as a library the base does not have,
//   marks every new case of the suite as failing to build.
// - A v3d_add_test entry that names its source through a variable other than
//   CMAKE_CURRENT_SOURCE_DIR is not read. When the change added that source, the
//   configure stops, and the run exits with 2.
// - A changed existing case is not checked. Only cases whose names are new are.
// - A case that also fails at the head revision is reported as failing. The normal test run
//   catches that.

import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import process from "node:process";

import { options, runGate } from "./lib/cli.ts";
import { GateError, commit, mergeBase } from "./lib/git.ts";
import { ROOT } from "./lib/tree.ts";
import { buildAll, buildSuite, configure, dropNote } from "./failsfirst/build.ts";
import { findNewCases } from "./failsfirst/cases.ts";
import { listedSources, suiteMap } from "./failsfirst/cmake.ts";
import { developerEnvironment, readCache } from "./failsfirst/environment.ts";
import { printStated, printTable } from "./failsfirst/report.ts";
import { changedFiles } from "./failsfirst/repo.ts";
import { runCase } from "./failsfirst/run.ts";
import { type Case, type Change, FAILS_TO_BUILD, NOT_RUN, PASSES, SKIPPED } from "./failsfirst/types.ts";
import { makeWorktree, removeWorktree } from "./failsfirst/worktree.ts";

const DEFAULT_BUILD = path.join("out", "build", "x64-Debug");

export type Options = {
    base: string | null;
    commit: string | null;
    list: boolean;
    keep: boolean;
    scratch: string | null;
    timeout: number;
    build: string;
};

function check(cases: Case[], base: string, head: string, changes: Change[], options: Options): void {
    const scratch = options.scratch ??
        path.join(os.tmpdir(), "v3d-failsfirst", `${base.slice(0, 8)}-${head.slice(0, 8)}`);
    if (fs.existsSync(scratch)) {
        throw new GateError(`${scratch} exists; remove it or pass --scratch`);
    }
    // read before the scratch directory exists, so a build that is not configured leaves nothing
    const cache = readCache(options.build);
    fs.mkdirSync(scratch, { recursive: true });
    const worktree = path.join(scratch, "src");
    const build = path.join(scratch, "build");
    let made = false;
    try {
        // inside the try, so that a developer environment that will not start removes the scratch
        // directory on the way out
        const env = developerEnvironment(scratch);
        made = true;
        const dropped = makeWorktree(head, base, changes, worktree);
        const added = new Set(changes.filter((c) => c.status === "A").map((c) => c.file));
        configure(worktree, build, options.build, cache, env, scratch, head, added, dropped);
        const bySuite = new Map<string, Case[]>();
        for (const c of cases) {
            if (!bySuite.has(c.suite)) {
                bySuite.set(c.suite, []);
            }
            bySuite.get(c.suite)!.push(c);
        }
        const listed = listedSources(head);
        const headMap = suiteMap(head);
        buildAll([...bySuite.keys()].sort(), build, env, scratch);
        for (const suite of [...bySuite.keys()].sort()) {
            const suiteCases = bySuite.get(suite)!;
            const directory = headMap.get(suiteCases[0].file)!.directory;
            // a base that does not add the head suite's directory, such as one where the change
            // moved it, may still build a binary of that name from somewhere else
            if (!fs.existsSync(path.join(build, ...directory.split("/"), "CMakeFiles", `v3dtest_${suite}.dir`))) {
                for (const c of suiteCases) {
                    c.result = FAILS_TO_BUILD;
                    c.note = `the base does not configure ${directory}`;
                }
                continue;
            }
            const sources = listed.filter((s) => s.suite === suite).map((s) => s.source);
            const executable = buildSuite(suite, directory, sources, suiteCases, worktree, build, env, scratch);
            for (const c of suiteCases) {
                if (c.result === FAILS_TO_BUILD && !c.note && dropped.has(suite)) {
                    c.note = dropNote(dropped.get(suite)!);
                }
                if (c.result !== null) {
                    continue;
                }
                if (executable === null) {
                    c.result = FAILS_TO_BUILD;
                } else {
                    console.log(`running ${suite} ${c.path}`);
                    runCase(c, executable, env, options.timeout);
                }
            }
        }
    } finally {
        if (options.keep) {
            console.log(`kept ${scratch}`);
        } else {
            if (made) {
                removeWorktree(worktree);
            }
            fs.rmSync(scratch, { recursive: true, force: true });
        }
    }
}

const USAGE = `usage: node scripts/failsfirst.ts [--base REF | --commit SHA] [--list] [--keep]
                                  [--scratch DIR] [--timeout SECONDS] [--build DIR]

Check that the test cases a changeset adds fail without its code.

  --base REF         the revision before the changeset
                     (default: the merge base of HEAD with origin/main, or with main)
  --commit SHA       check one commit, against its first parent
  --list             print the new cases and their suites, and build nothing
  --keep             keep the worktree and its build
  --scratch DIR      the directory for the worktree and the build
                     (default: a directory under the system temporary directory)
  --timeout SECONDS  seconds allowed for one case (default: 900)
  --build DIR        the configured build whose packages and settings are reused
                     (default: out/build/x64-Debug)`;

// The options, or null when the command line asks for the usage.
export function parseOptions(argv: string[]): Options | null {
    const { values } = options(USAGE, {
        base: { type: "string" }, commit: { type: "string" }, scratch: { type: "string" }, build: { type: "string" },
        timeout: { type: "string" }, list: { type: "boolean" }, keep: { type: "boolean" }, help: { type: "boolean", short: "h" },
    }, false, argv);
    if (values.help === true) {
        return null;
    }
    const timeout = values.timeout ?? "900";
    // a whole number of seconds, written in digits alone
    if (!/^\d+$/.test(timeout) || Number(timeout) === 0) {
        throw new GateError("--timeout needs a whole number of seconds");
    }
    return {
        base: values.base ?? null,
        commit: values.commit ?? null,
        list: values.list === true,
        keep: values.keep === true,
        scratch: values.scratch ?? null,
        timeout: Number(timeout),
        build: path.resolve(ROOT, values.build ?? DEFAULT_BUILD),
    };
}

function main(): number {
    const options = parseOptions(process.argv.slice(2));
    if (options === null) {
        console.log(USAGE);
        return 0;
    }
    let base: string;
    let head: string;
    if (options.commit !== null) {
        if (options.base !== null) {
            throw new GateError("--commit and --base cannot be combined");
        }
        head = commit(options.commit);
        base = commit(head + "^");
    } else {
        head = commit("HEAD");
        base = options.base !== null ? commit(options.base) : mergeBase();
    }

    const changes = changedFiles(base, head);
    const { cases, unregistered } = findNewCases(base, head, changes);
    console.log(`changeset ${base.slice(0, 9)}..${head.slice(0, 9)}: ${changes.length} files changed, ${cases.length} new test cases`);
    for (const file of unregistered) {
        console.log(`warning: ${file} has cases but no v3d_add_test list names it`);
    }
    if (cases.length === 0) {
        return 0;
    }
    if (options.list) {
        printTable(cases, false);
        printStated(cases);
        return 0;
    }

    check(cases, base, head, changes, options);
    console.log();
    printTable(cases, true);
    printStated(cases);
    const weak = cases.filter((c) => c.result === PASSES);
    const unrun = cases.filter((c) => c.result === NOT_RUN || c.result === SKIPPED);
    if (weak.length > 0) {
        console.log(`\n${weak.length} case(s) pass without the change. Each is a weak test.`);
        return 1;
    }
    if (unrun.length > 0) {
        console.log(`\n${unrun.length} case(s) were not run, so they are not checked.`);
        return 2;
    }
    return 0;
}

runGate(import.meta.url, main);
