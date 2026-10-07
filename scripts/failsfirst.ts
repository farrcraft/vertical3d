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
//
// Node runs this file directly, with its types stripped. It uses only Node's built-in modules
// and the lexer beside it.
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
// 3. Configures the worktree into its own build directory and builds only the affected suites.
//    The configure reuses the packages the main build already installed: it points
//    VCPKG_INSTALLED_DIR at that install, turns VCPKG_MANIFEST_INSTALL off, and uses the
//    checkout's vcpkg toolchain file, so nothing is installed. The values come from the main
//    build's CMakeCache.txt. vendor/libnoise is linked into the worktree with a directory
//    junction, because voxel links its prebuilt library. /WX is off, so that only an error
//    counts as a failure to build.
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
//                        rest are built and run.
//     PASSES - weak      the case passes without the change. It does not test the change.
//     passes, stated     the case passes without the change, and its doc comment says why.
//     skipped (no GPU)   the render_device binary found no Vulkan device and exited with 77.
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

import { spawnSync } from "node:child_process";
import * as fs from "node:fs";
import * as os from "node:os";
import * as path from "node:path";
import process from "node:process";

import { blankComments } from "./lexer.ts";
import { isEntryPoint } from './entry.ts';

const ROOT = path.resolve(import.meta.dirname, "..");
const MAIN_BUILD = path.join(ROOT, "out", "build", "x64-Debug");
const MAX_BUFFER = 512 * 1024 * 1024;

const FAILS = "fails";
const FAILS_TO_BUILD = "fails to build";
const PASSES = "PASSES - weak";
const PASSES_STATED = "passes, stated";
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

// What Boost.Test prints when a check, a requirement or a whole case fails, including a case
// that threw or crashed inside the framework.
const BOOST_FAILURE = /\*\*\* \d+ failures? (?:is|are) detected|\berror: in "|\bfailure occurred/i;

// The line in a case's doc comment that says why it passes without the change. It starts the
// text of the comment line, after the comment marker and any space.
const STATED = /^Passes before the change:\s*/;
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
type ListedSource = { suite: string; directory: string; source: string };
// A source dropped from a suite's list in the worktree, and whether the change added it.
type Drop = { source: string; added: boolean };
type Env = Record<string, string>;
export type Found = { path: string; stated: string | null };

type Case = {
    path: string;            // the --run_test path: suite names and the case name
    suite: string;           // the ctest suite, built as v3dtest_<suite>
    file: string;            // the test source, relative to the repository root
    stated: string | null;   // why the case passes without the change, from its doc comment
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

// Runs git with every path taken literally, so that a file name with a glob character in it
// names only that file.
function git(args: string[], cwd: string = ROOT, check: boolean = true): { code: number; stdout: string } {
    const done = spawnSync("git", ["--literal-pathspecs", ...args], { cwd, encoding: "utf8", maxBuffer: MAX_BUFFER });
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

// The reason a case's doc comment gives for passing without the change, or null. The doc
// comment is the comment that ends just before the case's macro: one block comment, or a run
// of // lines.
function statedReason(text: string, at: number): string | null {
    const before = text.slice(0, at).trimEnd();
    let comment: string;
    if (before.endsWith("*/")) {
        const start = before.lastIndexOf("/*");
        if (start < 0) {
            return null;
        }
        comment = before.slice(start);
    } else {
        const lines = before.split("\n");
        const kept: string[] = [];
        while (lines.length > 0 && lines[lines.length - 1].trim().startsWith("//")) {
            kept.unshift(lines.pop() as string);
        }
        comment = kept.join("\n");
    }
    const lines = comment.split(/\r?\n/).map((line) => line.trim()
        .replace(/^\/\*+|^\/\/+|^\*+(?!\/)/, "").replace(/\*+\/$/, "").trim());
    const first = lines.findIndex((line) => STATED.test(line));
    if (first < 0) {
        return null;
    }
    const reason: string[] = [lines[first].replace(STATED, "")];
    for (const line of lines.slice(first + 1)) {
        if (line === "") {
            break;
        }
        reason.push(line);
    }
    const joined = reason.join(" ").replace(/\s+/g, " ").trim();
    return joined === "" ? null : joined;
}

// The --run_test path of every case in a test source, with the reason its doc comment gives
// for passing without the change.
export function casesIn(text: string): Found[] {
    const found: Found[] = [];
    const suites: string[] = [];
    for (const match of blankComments(text).matchAll(TOKEN)) {
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
            found.push({ path: [...suites, name].join("/"), stated: statedReason(text, match.index) });
        }
    }
    return found;
}

// One entry of a v3d_add_test list: a quoted string, or a word.
const LIST_TOKEN = /"([^"]+)"|([^\s()"]+)/g;

// The file a v3d_add_test entry in <directory>/CMakeLists.txt names, relative to the repository
// root, or null for an entry that depends on a variable other than CMAKE_CURRENT_SOURCE_DIR.
function sourceOf(directory: string, entry: string): string | null {
    const spelled = entry.replace(/^\$\{CMAKE_CURRENT_SOURCE_DIR\}\//, "");
    if (spelled.includes("$")) {
        return null;
    }
    return path.posix.normalize(path.posix.join(directory, spelled));
}

// Every source named by a v3d_add_test list at the revision, with its suite and the directory
// of the tests/CMakeLists.txt that names it. A source two suites name appears once for each.
function listedSources(revision: string): ListedSource[] {
    const listed: ListedSource[] = [];
    const files = git(["ls-tree", "-r", "-z", "--name-only", revision]).stdout.split("\0");
    for (const file of files) {
        if (path.posix.basename(file) !== "CMakeLists.txt" || !isTestFile(file)) {
            continue;
        }
        const text = (show(revision, file) ?? "").replace(/#[^\n]*/g, "");
        const directory = path.posix.dirname(file);
        for (const match of text.matchAll(ADD_TEST)) {
            const suite = match[1];
            for (const token of match[2].matchAll(LIST_TOKEN)) {
                const source = sourceOf(directory, token[1] ?? token[2]);
                if (source !== null) {
                    listed.push({ suite, directory, source });
                }
            }
        }
    }
    return listed;
}

// Every test source named by a v3d_add_test list at the revision, with its suite.
function suiteMap(revision: string): Map<string, SuiteEntry> {
    const mapping = new Map<string, SuiteEntry>();
    for (const { suite, directory, source } of listedSources(revision)) {
        mapping.set(source, { suite, directory });
    }
    return mapping;
}

function findNewCases(base: string, head: string, changes: Change[]): { cases: Case[]; unregistered: string[] } {
    const headMap = suiteMap(head);
    const baseMap = suiteMap(base);
    const touched = [...new Set(changes.map((c) => c.file).filter((f) => isTestFile(f) && isSource(f)))].sort();

    const collect = (revision: string, mapping: Map<string, SuiteEntry>): Map<string, Map<string, [string, string | null]>> => {
        const bySuite = new Map<string, Map<string, [string, string | null]>>();
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
            for (const { path: name, stated } of casesIn(text)) {
                bySuite.get(entry.suite)!.set(name, [file, stated]);
            }
        }
        return bySuite;
    };

    const now = collect(head, headMap);
    const before = collect(base, baseMap);
    const cases: Case[] = [];
    for (const suite of [...now.keys()].sort()) {
        for (const [name, [file, stated]] of now.get(suite)!) {
            if (!before.get(suite)?.has(name)) {
                cases.push({ path: name, suite, file, stated, result: null, note: "" });
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

// The reason each case that states one gives for passing without the change.
function printStated(cases: Case[]): void {
    const stated = cases.filter((c) => c.stated !== null);
    if (stated.length === 0) {
        return;
    }
    console.log();
    console.log("Stated to pass before the change:");
    for (const c of stated) {
        console.log(`  ${c.suite} ${c.path}: ${c.stated}`);
    }
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

// Removes a source from the v3d_add_test(<suite> ...) list in the worktree's
// <directory>/CMakeLists.txt, whether the list quotes it or not. Returns whether it was there.
function dropSource(worktree: string, directory: string, suite: string, source: string): boolean {
    const file = path.join(worktree, directory, "CMakeLists.txt");
    if (!fs.existsSync(file)) {
        return false;
    }
    const content = fs.readFileSync(file, "utf8");
    // comments are blanked to spaces, so an offset in the copy is the same offset in the file
    const uncommented = content.replace(/#[^\n]*/g, (comment) => " ".repeat(comment.length));
    const call = new RegExp(`v3d_add_test\\s*\\(\\s*${escapeRegExp(suite)}\\b([\\s\\S]*?)\\)`).exec(uncommented);
    if (!call) {
        return false;
    }
    const listStart = call.index + call[0].length - call[1].length - 1;
    for (const token of call[1].matchAll(LIST_TOKEN)) {
        if (sourceOf(directory, token[1] ?? token[2]) === source) {
            const at = listStart + token.index;
            fs.writeFileSync(file, content.slice(0, at) + content.slice(at + token[0].length));
            return true;
        }
    }
    return false;
}

// Makes the worktree: the head revision with every file outside tests/ at its base version.
// Returns, for each suite, the sources dropped from its list because the change added them.
function makeWorktree(head: string, base: string, changes: Change[], worktree: string): Map<string, Drop[]> {
    console.log(`worktree: ${worktree}`);
    git(["worktree", "add", "--detach", worktree, head]);
    // a deleted file comes back even under tests/, because a base CMakeLists.txt may add the
    // test directory it was in; a head list does not name it, so it builds into nothing
    const restore = changes.filter((c) => c.status === "D" || (!isTestFile(c.file) && c.status !== "A")).map((c) => c.file);
    const remove = changes.filter((c) => !isTestFile(c.file) && c.status === "A").map((c) => c.file);
    for (let i = 0; i < restore.length; i += 100) {
        git(["checkout", base, "--", ...restore.slice(i, i + 100)], worktree);
    }
    for (let i = 0; i < remove.length; i += 100) {
        git(["rm", "-q", "--", ...remove.slice(i, i + 100)], worktree);
    }
    console.log(`restored ${restore.length} files to ${base.slice(0, 9)}, removed ${remove.length} non-test files it added`);

    // a head list that names a source the change added would stop the configure
    const removed = new Set(remove);
    const dropped = new Map<string, Drop[]>();
    for (const { suite, directory, source } of listedSources(head)) {
        if (removed.has(source) && dropSource(worktree, directory, suite, source)) {
            console.log(`dropped ${source} from v3dtest_${suite}: the change added it`);
            dropped.set(suite, [...(dropped.get(suite) ?? []), { source, added: true }]);
        }
    }

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
    return dropped;
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

// The sources a failed configure reports it cannot find, as the CMakeLists.txt spelled them.
function missingSources(output: string): string[] {
    return [...output.matchAll(/Cannot find source file:\s*\r?\n(?:\s*\r?\n)?\s*(\S[^\r\n]*)/g)].map((m) => m[1].trim());
}

// Configures the worktree. A source the configure cannot find is dropped from each head list that
// names it and resolves to a file the worktree lacks, and the configure runs again. A list in
// another directory that spells the name the same way names a different file, and keeps it. The
// drops are added to dropped, marked with whether the change added the source.
function configure(worktree: string, build: string, cache: Map<string, string>, env: Env, scratch: string,
                   head: string, added: Set<string>, dropped: Map<string, Drop[]>): void {
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
    const listed = listedSources(head);
    for (let attempt = 1; ; attempt++) {
        console.log(`configuring ${build}`);
        const { code, output } = runLogged(command, path.join(scratch, `configure-${attempt}.log`), env);
        if (code === 0) {
            return;
        }
        let progress = false;
        for (const name of missingSources(output)) {
            const named = name.replaceAll("\\", "/");
            for (const { suite, directory, source } of listed) {
                // the configure prints the entry as the list spelled it, or as an absolute path
                const full = path.isAbsolute(name) ?
                    path.relative(worktree, name).split(path.sep).join("/") : sourceOf(directory, named);
                if (full !== source || fs.existsSync(path.join(worktree, ...source.split("/")))) {
                    continue;
                }
                if (dropSource(worktree, directory, suite, source)) {
                    console.log(`dropped ${source} from v3dtest_${suite}: the configure cannot find it`);
                    dropped.set(suite, [...(dropped.get(suite) ?? []), { source, added: added.has(source) }]);
                    progress = true;
                }
            }
        }
        if (!progress || attempt >= 10) {
            throw new Failure("the worktree did not configure:\n" + tail(output));
        }
    }
}

// The suite's own sources that failed to compile, or whose objects name a symbol the link of the
// suite could not find. Null when anything else failed, or a failure names no source of the suite.
function failedSources(output: string, directory: string, suite: string, sources: string[]): string[] | null {
    const failed: string[] = [];
    const marker = `CMakeFiles/v3dtest_${suite}.dir/`;
    const executable = `v3dtest_${suite}.exe`;
    let linked = false;
    for (const match of output.matchAll(/^FAILED: (?:\[code=\d+\] )?(\S+)/gm)) {
        const outputFile = match[1].replaceAll("\\", "/");
        if (path.posix.basename(outputFile) === executable) {
            linked = true;
            continue;
        }
        if (!outputFile.includes(marker) || !outputFile.endsWith(".obj")) {
            return null;
        }
        const relative = outputFile.split(marker)[1].slice(0, -".obj".length)
            .split("/").map((part) => (part === "__" ? ".." : part)).join("/");
        failed.push(path.posix.normalize(path.posix.join(directory, relative)));
    }
    if (!linked) {
        return failed;
    }
    // an unresolved symbol names the object that refers to it; any other link error is not a
    // source's
    if (/error LNK(?!2001|2019|1120)\d+/.test(output)) {
        return null;
    }
    const objects = new Set<string>();
    for (const match of output.matchAll(/^\s*(\S[^\r\n]*?\.obj) : error LNK(?:2001|2019)/gim)) {
        objects.add(path.posix.basename(match[1].replaceAll("\\", "/")).toLowerCase());
    }
    if (objects.size === 0) {
        return null;
    }
    for (const object of objects) {
        const owners = sources.filter((s) => (path.posix.basename(s) + ".obj").toLowerCase() === object);
        if (owners.length !== 1) {
            return null;
        }
        failed.push(owners[0]);
    }
    return failed;
}

// Why a suite that lost sources from its list may not build: the sources the change added, and
// those the worktree lacks for another reason.
function dropNote(drops: Drop[]): string {
    const added = drops.filter((d) => d.added).map((d) => d.source);
    const missing = drops.filter((d) => !d.added).map((d) => d.source);
    const parts: string[] = [];
    if (added.length > 0) {
        parts.push(`the suite needs ${added.join(", ")}, which the change added`);
    }
    if (missing.length > 0) {
        parts.push(`the suite lists ${missing.join(", ")}, which the worktree does not have`);
    }
    return parts.join("; ");
}

function escapeRegExp(text: string): string {
    return text.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
}

function markUnbuilt(cases: Case[]): void {
    for (const c of cases) {
        if (c.result === null) {
            c.result = FAILS_TO_BUILD;
        }
    }
}

// Builds v3dtest_<suite>. Marks the cases that cannot be built or run, and returns the
// executable, or null when there is none to run.
function buildSuite(suite: string, directory: string, sources: string[], cases: Case[], worktree: string,
                    build: string, env: Env, scratch: string): string | null {
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
        // a test file that does not build is dropped whether or not it has new cases: one with
        // none loses nothing, and the rest of the suite can still run
        const failed = failedSources(output, directory, suite, sources);
        if (!failed || failed.length === 0 || failed.some((f) => !isTestFile(f) || dropped.has(f))) {
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
                    c.note = "its file does not compile or link";
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
    for (const c of cases) {
        if (c.result === null) {
            c.result = NOT_RUN;
            c.note = `the build made no ${target}.exe`;
        }
    }
    return null;
}

// An exit status as Windows reports it: an NTSTATUS such as 0xC0000135 in hexadecimal.
function describeExit(status: number | null, signal: string | null): string {
    if (signal !== null) {
        return `signal ${signal}`;
    }
    if (status === null) {
        return "no status";
    }
    return status > 0xFFFF ? `0x${(status >>> 0).toString(16).toUpperCase()}` : String(status);
}

type Outcome = { error?: Error; status: number | null; signal: string | null; output: string };

function runCase(c: Case, executable: string, env: Env, timeout: number): void {
    const done = spawnSync(executable, [`--run_test=${c.path}`, "--detect_memory_leaks=0"], {
        cwd: path.dirname(executable), env, encoding: "utf8", maxBuffer: MAX_BUFFER, timeout: timeout * 1000,
    });
    const output = (done.stdout ?? "") + (done.stderr ?? "");
    classify(c, { error: done.error, status: done.status, signal: done.signal, output }, timeout);
}

// Sets the case's result from how its run ended. Only a failure Boost.Test reported counts as
// failing.
function classify(c: Case, done: Outcome, timeout: number): void {
    if (done.error) {
        c.result = NOT_RUN;
        const code = (done.error as NodeJS.ErrnoException).code;
        c.note = code === "ETIMEDOUT" ? `timed out after ${timeout}s` : `could not start: ${done.error.message}`;
        return;
    }
    const output = done.output;
    if (done.status === 0) {
        c.result = c.stated === null ? PASSES : PASSES_STATED;
    } else if (done.status === SKIP_RETURN_CODE && c.suite === "render_device") {
        c.result = SKIPPED;
    } else if (/no test cases matching filter|test setup error/i.test(output)) {
        c.result = NOT_RUN;
        c.note = "the binary has no such case";
    } else if (BOOST_FAILURE.test(output)) {
        c.result = FAILS;
    } else {
        // a binary that cannot load a DLL, or that dies before the framework reports, has not
        // shown that the case fails
        c.result = NOT_RUN;
        c.note = `exited with ${describeExit(done.status, done.signal)} and reported no failed check`;
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
        made = true;
        const dropped = makeWorktree(head, base, changes, worktree);
        const added = new Set(changes.filter((c) => c.status === "A").map((c) => c.file));
        configure(worktree, build, cache, env, scratch, head, added, dropped);
        const bySuite = new Map<string, Case[]>();
        for (const c of cases) {
            if (!bySuite.has(c.suite)) {
                bySuite.set(c.suite, []);
            }
            bySuite.get(c.suite)!.push(c);
        }
        const listed = listedSources(head);
        const headMap = suiteMap(head);
        for (const suite of [...bySuite.keys()].sort()) {
            const suiteCases = bySuite.get(suite)!;
            const directory = headMap.get(suiteCases[0].file)!.directory;
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
    } catch (error) {
        if (error instanceof Failure) {
            console.error(`failsfirst: ${error.message}`);
            return 2;
        }
        throw error;
    }
}

if (isEntryPoint(import.meta.url)) {
    process.exitCode = main();
}
