// Check that the test cases a changeset adds fail without the changeset's code.
//
// A test that passes without the code it claims to test is a weak test. This script finds the
// Boost.Test cases a changeset adds and runs each one against the changeset's tests with the
// base version of everything else. Every new case should fail there, or fail to build.
//
//     node scripts/failsfirst.ts                    the branch, from its merge base with main
//     node scripts/failsfirst.ts --base 13a9557     the commits after 13a9557
//     node scripts/failsfirst.ts --commit 5ff63287  one commit, against its parent
//     node scripts/failsfirst.ts --list             only print the new cases and their suites
//
// Node runs this file directly, with its types stripped. It uses only Node's built-in modules.
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
//    added outside tests/ is removed. Everything under a tests/ directory keeps its head version:
//    the test sources, their CMakeLists.txt, and the fixtures under tests/data, tests/fixtures
//    and tests/device/data.
// 3. Configures the worktree into its own build directory and builds only the affected suites.
//    The configure reuses the packages the main build already installed: it points
//    VCPKG_INSTALLED_DIR at that install, turns VCPKG_MANIFEST_INSTALL off, and uses the
//    checkout's vcpkg toolchain file, so nothing is installed. The values come from the main
//    build's CMakeCache.txt. vendor/libnoise is linked into the worktree with a directory
//    junction, because voxel links its prebuilt library. /WX is off, so that only an error
//    counts as a failure to build.
// 4. Runs each new case alone, as v3dtest_<suite>.exe --run_test=<path>, from the executable's
//    directory so that its fixtures resolve.
// 5. Prints one row per case and exits 1 when any case passed. The worktree and its build are
//    removed unless --keep is given.
//
// The results:
//
//     fails              the case fails without the change. This is the expected result.
//     fails to build     the suite does not compile or link without the change, usually because
//                        the case uses an API the change added. This counts as failing. When only
//                        some test files fail to compile, they are dropped from the suite in the
//                        worktree and the rest are built and run.
//     PASSES - weak      the case passes without the change. It does not test the change.
//     skipped (no GPU)   the render_device binary found no Vulkan device and exited with 77.
//     not run            the binary did not find the case, or the case timed out.
//
// The build environment is the one scripts\build.cmd enters: vcvars64.bat under Visual Studio 18
// or 2022 Community, or the file V3D_VCVARS names. A cold build of a suite compiles every library
// it links, which takes several minutes for the render suites.
//
// Limits:
//
// - Only committed revisions are compared. Uncommitted changes in the checkout are ignored.
// - A render_device case needs a Vulkan device. Without one the binary skips, and the case is
//   reported as skipped, not as passing or failing.
// - A case can fail without the change only because of a fixture the change added, such as a
//   file the old code cannot read. That counts as failing here, but it shows that the fixture
//   is new, not that the test checks the new code. Read such a case by hand.
// - A whole test file is the unit dropped after a compile error. A new case that would have
//   compiled is reported as failing to build when it shares a file with one that did not.
// - A changed existing case is not checked. Only cases whose names are new are.
// - A case that also fails at the head revision is reported as failing. The normal test run
//   catches that.

import { spawnSync } from "node:child_process";
import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import process from "node:process";

const ROOT = path.resolve(import.meta.dirname, "..");
const MAIN_BUILD = path.join(ROOT, "out", "build", "x64-Debug");
const MAX_BUFFER = 512 * 1024 * 1024;

const FAILS = "fails";
const FAILS_TO_BUILD = "fails to build";
const PASSES = "PASSES - weak";
const SKIPPED = "skipped (no GPU)";
const NOT_RUN = "not run";

// A Boost.Test executable that found no usable device before starting. Only render_device
// returns it.
const SKIP_RETURN_CODE = 77;

// The tokens that open and close a Boost.Test suite and that declare a case. A data case with
// a fixture names the case in its second argument, and every other macro in its first.
const TOKEN = new RegExp(
    "\\b(BOOST_AUTO_TEST_SUITE_END|BOOST_AUTO_TEST_SUITE|BOOST_FIXTURE_TEST_SUITE" +
    "|BOOST_AUTO_TEST_CASE_TEMPLATE|BOOST_FIXTURE_TEST_CASE_TEMPLATE" +
    "|BOOST_DATA_TEST_CASE_F|BOOST_AUTO_TEST_CASE|BOOST_FIXTURE_TEST_CASE|BOOST_DATA_TEST_CASE)" +
    "\\s*\\(([^)]*)", "g");
const NAME_ARGUMENT: Record<string, number> = {
    BOOST_AUTO_TEST_SUITE: 0,
    BOOST_FIXTURE_TEST_SUITE: 0,
    BOOST_AUTO_TEST_CASE: 0,
    BOOST_FIXTURE_TEST_CASE: 0,
    BOOST_DATA_TEST_CASE: 0,
    BOOST_AUTO_TEST_CASE_TEMPLATE: 0,
    BOOST_FIXTURE_TEST_CASE_TEMPLATE: 0,
    BOOST_DATA_TEST_CASE_F: 1,
};

const ADD_TEST = /v3d_add_test\s*\(\s*(\w+)([\s\S]*?)\)/g;
const SOURCE_SUFFIXES = [".cpp", ".cxx", ".cc", ".c"];

// The developer environment, as scripts\build.cmd enters it, followed by a dump of the
// environment it leaves, which the build steps below are given.
const ENVIRONMENT_CMD = [
    "@echo off",
    'if not defined V3D_VCVARS set "V3D_VCVARS=%ProgramFiles%\\Microsoft Visual Studio\\18\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"',
    'if not exist "%V3D_VCVARS%" set "V3D_VCVARS=%ProgramFiles%\\Microsoft Visual Studio\\2022\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"',
    'if not exist "%V3D_VCVARS%" (',
    "    echo Could not find vcvars64.bat. Set V3D_VCVARS to it, or see docs/contributing/Build.md. 1>&2",
    "    exit /b 1",
    ")",
    'call "%V3D_VCVARS%" >nul || exit /b 1',
    "set",
    "",
].join("\r\n");

type Change = { status: string; file: string };
type SuiteEntry = { suite: string; directory: string };
type Env = Record<string, string>;

type Case = {
    path: string;      // the --run_test path: suite names and the case name
    suite: string;     // the ctest suite, built as v3dtest_<suite>
    file: string;      // the test source, relative to the repository root
    result: string | null;
    note: string;
};

type Options = {
    base: string | null;
    commit: string | null;
    list: boolean;
    keep: boolean;
    scratch: string | null;
    timeout: number;
};

class Failure extends Error {}

function git(args: string[], cwd: string = ROOT, check: boolean = true): { code: number; stdout: string } {
    const done = spawnSync("git", args, { cwd, encoding: "utf8", maxBuffer: MAX_BUFFER });
    const code = done.status ?? 1;
    if (check && code !== 0) {
        throw new Failure(`git ${args.join(" ")} failed:\n${(done.stderr ?? "").trim()}`);
    }
    return { code, stdout: done.stdout ?? "" };
}

function resolve(ref: string): string {
    return git(["rev-parse", "--verify", ref + "^{commit}"]).stdout.trim();
}

// The file's text at the revision, or null when it does not exist there.
function show(revision: string, file: string): string | null {
    const done = git(["show", `${revision}:${file}`], ROOT, false);
    return done.code === 0 ? done.stdout : null;
}

function isTestFile(file: string): boolean {
    return file.split("/").slice(0, -1).includes("tests");
}

function isSource(file: string): boolean {
    return SOURCE_SUFFIXES.some((suffix) => file.endsWith(suffix));
}

// Every file the changeset touched. A rename is a delete and an add.
function changedFiles(base: string, head: string): Change[] {
    const fields = git(["diff", "--name-status", "--no-renames", "-z", base, head]).stdout
        .split("\0").filter((f) => f.length > 0);
    const changes: Change[] = [];
    for (let i = 0; i + 1 < fields.length; i += 2) {
        changes.push({ status: fields[i][0], file: fields[i + 1] });
    }
    return changes;
}

function stripComments(text: string): string {
    const blocks = text.replace(/\/\*[\s\S]*?\*\//g, (m) => "\n".repeat(m.split("\n").length - 1));
    return blocks.replace(/\/\/[^\n]*/g, "");
}

// The --run_test path of every case in a test source.
function casesIn(text: string): string[] {
    const found: string[] = [];
    const suites: string[] = [];
    for (const match of stripComments(text).matchAll(TOKEN)) {
        const macro = match[1];
        if (macro === "BOOST_AUTO_TEST_SUITE_END") {
            suites.pop();
            continue;
        }
        const args = match[2].split(",").map((a) => a.trim());
        const index = NAME_ARGUMENT[macro];
        if (index >= args.length || !/^\w+$/.test(args[index])) {
            continue;
        }
        const name = args[index];
        if (macro.endsWith("_SUITE")) {
            suites.push(name);
        } else {
            found.push([...suites, name].join("/"));
        }
    }
    return found;
}

// Every test source named by a v3d_add_test list at the revision, with its suite and the
// directory of the tests/CMakeLists.txt that names it.
function suiteMap(revision: string): Map<string, SuiteEntry> {
    const mapping = new Map<string, SuiteEntry>();
    const files = git(["ls-tree", "-r", "--name-only", revision]).stdout.split("\n");
    for (const file of files) {
        if (path.posix.basename(file) !== "CMakeLists.txt" || !isTestFile(file)) {
            continue;
        }
        const text = (show(revision, file) ?? "").replace(/#[^\n]*/g, "");
        const directory = path.posix.dirname(file);
        for (const match of text.matchAll(ADD_TEST)) {
            const suite = match[1];
            for (const token of match[2].matchAll(/"([^"]+)"|(\S+)/g)) {
                const source = token[1] ?? token[2];
                if (source.includes("$")) {
                    continue;
                }
                mapping.set(path.posix.normalize(path.posix.join(directory, source)), { suite, directory });
            }
        }
    }
    return mapping;
}

function findNewCases(base: string, head: string, changes: Change[]): { cases: Case[]; unregistered: string[] } {
    const headMap = suiteMap(head);
    const baseMap = suiteMap(base);
    const touched = [...new Set(changes.map((c) => c.file).filter((f) => isTestFile(f) && isSource(f)))].sort();

    const collect = (revision: string, mapping: Map<string, SuiteEntry>): Map<string, Map<string, string>> => {
        const bySuite = new Map<string, Map<string, string>>();
        for (const file of touched) {
            const entry = mapping.get(file);
            if (entry === undefined) {
                continue;
            }
            const text = show(revision, file);
            if (text === null) {
                continue;
            }
            if (!bySuite.has(entry.suite)) {
                bySuite.set(entry.suite, new Map());
            }
            for (const name of casesIn(text)) {
                bySuite.get(entry.suite)!.set(name, file);
            }
        }
        return bySuite;
    };

    const now = collect(head, headMap);
    const before = collect(base, baseMap);
    const cases: Case[] = [];
    for (const suite of [...now.keys()].sort()) {
        for (const [name, file] of now.get(suite)!) {
            if (!before.get(suite)?.has(name)) {
                cases.push({ path: name, suite, file, result: null, note: "" });
            }
        }
    }

    const unregistered: string[] = [];
    for (const file of touched) {
        if (!headMap.has(file)) {
            const text = show(head, file);
            if (text !== null && casesIn(text).length > 0) {
                unregistered.push(file);
            }
        }
    }
    return { cases, unregistered };
}

function defaultBase(): string {
    for (const ref of ["origin/main", "main"]) {
        const done = git(["merge-base", "HEAD", ref], ROOT, false);
        if (done.code === 0) {
            return done.stdout.trim();
        }
    }
    throw new Failure("no merge base with origin/main or main; pass --base");
}

function printTable(cases: Case[], withResult: boolean): void {
    const headers = ["case", "suite", "file", ...(withResult ? ["result"] : [])];
    const rows = cases.map((c) => {
        const row = [c.path, c.suite, c.file];
        if (withResult) {
            row.push((c.result ?? "") + (c.note ? ` (${c.note})` : ""));
        }
        return row;
    });
    const widths = headers.map((h, i) => Math.max(h.length, ...rows.map((r) => r[i].length)));
    const line = (cells: string[]): string => cells.map((cell, i) => cell.padEnd(widths[i])).join("  ");
    const out = [line(headers), line(widths.map((w) => "-".repeat(w))), ...rows.map(line)];
    process.stdout.write(out.join("\n") + "\n");
}

function readCache(build: string): Map<string, string> {
    const file = path.join(build, "CMakeCache.txt");
    if (!fs.existsSync(file)) {
        throw new Failure(`${build} is not configured; the worktree build reuses its packages`);
    }
    const values = new Map<string, string>();
    for (const line of fs.readFileSync(file, "utf8").split(/\r?\n/)) {
        const match = /^([A-Za-z_][\w-]*):[A-Z]+=(.*)$/.exec(line);
        if (match) {
            values.set(match[1], match[2]);
        }
    }
    return values;
}

function developerEnvironment(scratch: string): Env {
    const script = path.join(scratch, "environment.cmd");
    fs.writeFileSync(script, ENVIRONMENT_CMD);
    const done = spawnSync("cmd", ["/d", "/c", script], { encoding: "utf8", maxBuffer: MAX_BUFFER });
    if (done.status !== 0) {
        throw new Failure("could not enter the developer environment:\n" + (done.stderr ?? "").trim());
    }
    const env: Env = {};
    for (const line of done.stdout.split(/\r?\n/)) {
        const at = line.indexOf("=");
        if (at > 0) {
            env[line.slice(0, at)] = line.slice(at + 1);
        }
    }
    return env;
}

function runLogged(command: string[], log: string, env: Env): { code: number; output: string } {
    const fd = fs.openSync(log, "w");
    let done;
    try {
        done = spawnSync(command[0], command.slice(1), { env, stdio: ["ignore", fd, fd] });
    } finally {
        fs.closeSync(fd);
    }
    return { code: done.status ?? 1, output: fs.readFileSync(log, "utf8") };
}

function tail(text: string, lines: number = 40): string {
    return text.split(/\r?\n/).slice(-lines).join("\n");
}

function makeWorktree(head: string, base: string, changes: Change[], worktree: string): void {
    console.log(`worktree: ${worktree}`);
    git(["worktree", "add", "--detach", worktree, head]);
    const restore = changes.filter((c) => !isTestFile(c.file) && c.status !== "A").map((c) => c.file);
    const remove = changes.filter((c) => !isTestFile(c.file) && c.status === "A").map((c) => c.file);
    for (let i = 0; i < restore.length; i += 100) {
        git(["checkout", base, "--", ...restore.slice(i, i + 100)], worktree);
    }
    for (const file of remove) {
        git(["rm", "-q", "--", file], worktree);
    }
    console.log(`restored ${restore.length} non-test files to ${base.slice(0, 9)}, removed ${remove.length} it added`);

    // voxel links libnoise from vendor/libnoise/Debug, which is built in the checkout and is
    // not part of any commit.
    const noise = path.join(ROOT, "vendor", "libnoise");
    const link = path.join(worktree, "vendor", "libnoise");
    if (fs.existsSync(path.join(noise, "Debug"))) {
        if (fs.existsSync(link) && fs.readdirSync(link).length === 0) {
            fs.rmdirSync(link);
        }
        if (!fs.existsSync(link)) {
            fs.symlinkSync(noise, link, "junction");
        }
    }
}

function removeWorktree(worktree: string): void {
    const link = path.join(worktree, "vendor", "libnoise");
    // A junction is removed as a link. Deleting through it would delete the checkout's
    // libnoise build.
    try {
        if (fs.lstatSync(link).isSymbolicLink()) {
            fs.unlinkSync(link);
        }
    } catch {
        // there is no link to remove
    }
    if (git(["worktree", "remove", "--force", worktree], ROOT, false).code !== 0) {
        fs.rmSync(worktree, { recursive: true, force: true });
        git(["worktree", "prune"], ROOT, false);
    }
}

function configure(worktree: string, build: string, cache: Map<string, string>, env: Env, scratch: string): void {
    let toolchain = cache.get("CMAKE_TOOLCHAIN_FILE") ?? "vendor/vcpkg/scripts/buildsystems/vcpkg.cmake";
    if (!path.isAbsolute(toolchain)) {
        toolchain = path.join(ROOT, toolchain);
    }
    const installed = cache.get("VCPKG_INSTALLED_DIR") || path.join(MAIN_BUILD, "vcpkg_installed");
    const command = [
        "cmake", "-S", worktree, "-B", build, "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=" + (cache.get("CMAKE_BUILD_TYPE") ?? "Debug"),
        "-DCMAKE_TOOLCHAIN_FILE=" + toolchain.replaceAll("\\", "/"),
        "-DVCPKG_TARGET_TRIPLET=" + (cache.get("VCPKG_TARGET_TRIPLET") ?? "x64-windows"),
        "-DVCPKG_INSTALLED_DIR=" + installed.replaceAll("\\", "/"),
        "-DVCPKG_MANIFEST_INSTALL=OFF",
        "-DV3D_WARNINGS_AS_ERRORS=OFF",
    ];
    console.log(`configuring ${build}`);
    const { code, output } = runLogged(command, path.join(scratch, "configure.log"), env);
    if (code !== 0) {
        throw new Failure("the worktree did not configure:\n" + tail(output));
    }
}

// The suite's own sources that failed to compile, or null when anything else failed.
function failedSources(output: string, directory: string, suite: string): string[] | null {
    const failed: string[] = [];
    const marker = `CMakeFiles/v3dtest_${suite}.dir/`;
    for (const match of output.matchAll(/^FAILED: (?:\[code=\d+\] )?(\S+)/gm)) {
        const outputFile = match[1].replaceAll("\\", "/");
        if (!outputFile.includes(marker) || !outputFile.endsWith(".obj")) {
            return null;
        }
        const relative = outputFile.split(marker)[1].slice(0, -".obj".length)
            .split("/").map((part) => (part === "__" ? ".." : part)).join("/");
        failed.push(path.posix.normalize(path.posix.join(directory, relative)));
    }
    return failed;
}

function escapeRegExp(text: string): string {
    return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function dropSource(worktree: string, directory: string, suite: string, source: string): boolean {
    const file = path.join(worktree, directory, "CMakeLists.txt");
    const content = fs.readFileSync(file, "utf8");
    const quoted = `"${path.posix.relative(directory, source)}"`;
    const match = new RegExp(`v3d_add_test\\s*\\(\\s*${escapeRegExp(suite)}\\b[\\s\\S]*?\\)`).exec(content);
    if (!match || !match[0].includes(quoted)) {
        return false;
    }
    const block = match[0].replace(quoted, "");
    fs.writeFileSync(file, content.slice(0, match.index) + block + content.slice(match.index + match[0].length));
    return true;
}

function markUnbuilt(cases: Case[]): void {
    for (const c of cases) {
        if (c.result === null) {
            c.result = FAILS_TO_BUILD;
        }
    }
}

// Builds v3dtest_<suite>. Marks the cases that cannot be built, and returns the executable.
function buildSuite(suite: string, directory: string, cases: Case[], worktree: string, build: string,
                    env: Env, scratch: string): string | null {
    const target = "v3dtest_" + suite;
    const dropped = new Set<string>();
    for (;;) {
        console.log(`building ${target}`);
        const { code, output } = runLogged(["ninja", "-k", "0", "-C", build, target],
                                           path.join(scratch, `build-${suite}.log`), env);
        if (code === 0) {
            break;
        }
        if (output.includes("unknown target")) {
            for (const c of cases) {
                c.result = FAILS_TO_BUILD;
                c.note = "the suite does not exist without the change";
            }
            return null;
        }
        const failed = failedSources(output, directory, suite);
        const filesWithCases = new Set(cases.filter((c) => c.result === null).map((c) => c.file));
        if (!failed || failed.length === 0 || failed.some((f) => !filesWithCases.has(f) || dropped.has(f))) {
            console.log(tail(output, 20));
            markUnbuilt(cases);
            return null;
        }
        for (const source of failed) {
            if (!dropSource(worktree, directory, suite, source)) {
                markUnbuilt(cases);
                return null;
            }
            dropped.add(source);
            for (const c of cases) {
                if (c.file === source) {
                    c.result = FAILS_TO_BUILD;
                    c.note = "its file does not compile";
                }
            }
        }
        if (cases.every((c) => c.result !== null)) {
            return null;
        }
    }
    const executable = path.join(build, ...directory.split("/"), target + ".exe");
    if (fs.existsSync(executable)) {
        return executable;
    }
    for (const entry of fs.readdirSync(build, { recursive: true }) as string[]) {
        if (path.basename(entry) === target + ".exe") {
            return path.join(build, entry);
        }
    }
    return executable;
}

function runCase(c: Case, executable: string, env: Env, timeout: number): void {
    const done = spawnSync(executable, [`--run_test=${c.path}`, "--detect_memory_leaks=0"], {
        cwd: path.dirname(executable), env, encoding: "utf8", maxBuffer: MAX_BUFFER, timeout: timeout * 1000,
    });
    if (done.error && (done.error as NodeJS.ErrnoException).code === "ETIMEDOUT") {
        c.result = NOT_RUN;
        c.note = `timed out after ${timeout}s`;
        return;
    }
    const output = ((done.stdout ?? "") + (done.stderr ?? "")).toLowerCase();
    if (done.status === 0) {
        c.result = PASSES;
    } else if (done.status === SKIP_RETURN_CODE && c.suite === "render_device") {
        c.result = SKIPPED;
    } else if (output.includes("no test cases matching filter") || output.includes("test setup error")) {
        c.result = NOT_RUN;
        c.note = "the binary has no such case";
    } else {
        c.result = FAILS;
    }
}

function check(cases: Case[], base: string, head: string, changes: Change[], options: Options): void {
    const scratch = options.scratch ??
        path.join(os.tmpdir(), "v3d-failsfirst", `${base.slice(0, 8)}-${head.slice(0, 8)}`);
    if (fs.existsSync(scratch)) {
        throw new Failure(`${scratch} exists; remove it or pass --scratch`);
    }
    fs.mkdirSync(scratch, { recursive: true });
    const worktree = path.join(scratch, "src");
    const build = path.join(scratch, "build");
    const cache = readCache(MAIN_BUILD);
    const env = developerEnvironment(scratch);
    let made = false;
    try {
        makeWorktree(head, base, changes, worktree);
        made = true;
        configure(worktree, build, cache, env, scratch);
        const bySuite = new Map<string, Case[]>();
        for (const c of cases) {
            if (!bySuite.has(c.suite)) {
                bySuite.set(c.suite, []);
            }
            bySuite.get(c.suite)!.push(c);
        }
        const headMap = suiteMap(head);
        for (const suite of [...bySuite.keys()].sort()) {
            const suiteCases = bySuite.get(suite)!;
            const directory = headMap.get(suiteCases[0].file)!.directory;
            const executable = buildSuite(suite, directory, suiteCases, worktree, build, env, scratch);
            for (const c of suiteCases) {
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
                                  [--scratch DIR] [--timeout SECONDS]

Check that the test cases a changeset adds fail without its code.

  --base REF         the revision before the changeset
                     (default: the merge base of HEAD with origin/main, or with main)
  --commit SHA       check one commit, against its first parent
  --list             print the new cases and their suites, and build nothing
  --keep             keep the worktree and its build
  --scratch DIR      the directory for the worktree and the build
                     (default: a directory under the system temporary directory)
  --timeout SECONDS  seconds allowed for one case (default: 900)`;

function parseOptions(argv: string[]): Options {
    const options: Options = { base: null, commit: null, list: false, keep: false, scratch: null, timeout: 900 };
    for (let i = 0; i < argv.length; i++) {
        const arg = argv[i];
        const value = (): string => {
            if (i + 1 >= argv.length) {
                throw new Failure(`${arg} needs a value`);
            }
            return argv[++i];
        };
        if (arg === "--base") {
            options.base = value();
        } else if (arg === "--commit") {
            options.commit = value();
        } else if (arg === "--scratch") {
            options.scratch = value();
        } else if (arg === "--timeout") {
            options.timeout = Number.parseInt(value(), 10);
            if (!(options.timeout > 0)) {
                throw new Failure("--timeout needs a whole number of seconds");
            }
        } else if (arg === "--list") {
            options.list = true;
        } else if (arg === "--keep") {
            options.keep = true;
        } else if (arg === "-h" || arg === "--help") {
            console.log(USAGE);
            process.exit(0);
        } else {
            throw new Failure(`unknown argument ${arg}\n${USAGE}`);
        }
    }
    return options;
}

function main(): number {
    try {
        const options = parseOptions(process.argv.slice(2));
        let base: string;
        let head: string;
        if (options.commit) {
            if (options.base) {
                throw new Failure("--commit and --base cannot be combined");
            }
            head = resolve(options.commit);
            base = resolve(head + "^");
        } else {
            head = resolve("HEAD");
            base = options.base ? resolve(options.base) : defaultBase();
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
            return 0;
        }

        check(cases, base, head, changes, options);
        console.log();
        printTable(cases, true);
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
    } catch (error) {
        if (error instanceof Failure) {
            console.error(`failsfirst: ${error.message}`);
            return 2;
        }
        throw error;
    }
}

process.exitCode = main();
