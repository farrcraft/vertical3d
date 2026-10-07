/**
 * Check the linking rules in docs/contributing/Build.md and Testing.md for apps and suites.
 *
 * The script reads every CMakeLists.txt in the tree and checks each target that is not an api
 * library: every app, every app library such as v3dlib_moya, every app test suite and every api
 * test suite. A target's files are the sources its add_executable, add_library or v3d_add_test
 * call lists, and every header those include that is neither in api/ nor in an app library's
 * directory, followed transitively. A suite's files therefore include the ../src files it
 * compiles and the app headers those include. A header under a tests/ directory is a local
 * header, even under api/.
 *
 *   missing-link      a file includes the header of a v3dlib_* target that the target does
 *                     not name.
 *   extra-link        the target names a v3dlib_* target whose headers none of its files
 *                     include.
 *   namespace-use     a file names something in a v3d::<lib>:: namespace, and none of the
 *                     target's files includes a header of that library. The type may arrive
 *                     through another header; the rule is that a target includes what it uses.
 *   visibility        an app library's header includes a v3dlib_* target that the app library
 *                     links PRIVATE, or the app library links one PUBLIC that none of its
 *                     headers includes.
 *   package           an app or an app suite names a third-party package that is not the
 *                     exception. The exception is a package its own files include directly
 *                     that none of the v3dlib_* targets it links carries. A suite never names
 *                     Boost::unit_test_framework, because v3d_add_test links it. A variable
 *                     such as ${Boost_LIBRARIES} names every target it expands to: config-mode
 *                     Boost sets it to all the components the tree finds, and
 *                     ${PNG_LIBRARIES} and its kin name the package's target.
 *   package-unlinked  an app or an app suite includes a package header that no target it
 *                     links carries and that it does not name itself.
 *
 * Api libraries are not checked here. The configure checks them against the manifest in
 * cmake/v3dApiLibraries.cmake: what each one links, and whether a link is PUBLIC or PRIVATE.
 * examples/ is not checked either. It is a project outside the tree's build, and CI builds it
 * separately.
 *
 * Includes are found by reading the text, after comments and #if 0 blocks are removed. A
 * conditional include is counted whether or not its condition holds.
 *
 * Usage: node scripts/linkrule.ts
 *
 * Each violation prints as "path: rule: detail". The exit status is 1 when there is a
 * violation, 2 when the command is given an argument, and 0 otherwise.
 */

import { execFileSync } from 'node:child_process';
import { existsSync, readFileSync, statSync } from 'node:fs';
import * as path from 'node:path';

import { blankComments, codeLines } from './lexer.ts';
import { isEntryPoint } from './entry.ts';

const ROOT = path.resolve(import.meta.dirname, '..');

const SOURCE_SUFFIXES = ['.h', '.hpp', '.inl', '.c', '.cpp', '.cxx'];
const HEADER_SUFFIXES = ['.h', '.hpp', '.inl'];

// The header paths each package target provides, matched against the text of an include. The
// specific Boost components come before Boost::headers, which takes the rest of boost/.
const PACKAGE_HEADERS: Array<[string, RegExp]> = [
    ['Boost::filesystem', /^boost\/filesystem/],
    ['Boost::json', /^boost\/json/],
    ['Boost::program_options', /^boost\/program_options/],
    ['Boost::unit_test_framework', /^boost\/test\//],
    ['Boost::headers', /^boost\//],
    ['glm::glm', /^glm\//],
    ['EnTT::EnTT', /^entt\//],
    ['SDL3_mixer::SDL3_mixer', /^SDL3_mixer\//],
    ['SDL3::SDL3', /^SDL3\//],
    ['Freetype::Freetype', /^(ft2build\.h|freetype\/)/],
    ['PNG::PNG', /^png\.h$/],
    ['JPEG::JPEG', /^(jpeglib|jerror|jconfig|jmorecfg)\.h$/],
    ['Vulkan::Vulkan', /^vulkan\//],
    ['GPUOpen::VulkanMemoryAllocator', /^vk_mem_alloc/],
    ['spdlog::spdlog', /^(spdlog|fmt)\//],
    ['libnoise', /^(noise\.h|interp\.h|mathconsts\.h|noise\/)/],
];

// A package variable such as ${PNG_LIBRARIES} names the target of the package it is named after.
const VARIABLE_TARGETS: Record<string, string[]> = {
    PNG: ['PNG::PNG'],
    JPEG: ['JPEG::JPEG'],
    SDL3: ['SDL3::SDL3'],
    Vulkan: ['Vulkan::Vulkan'],
    Freetype: ['Freetype::Freetype'],
};

const LINK_KEYWORDS = new Set(['PUBLIC', 'PRIVATE', 'INTERFACE', 'LINK_PUBLIC', 'LINK_PRIVATE']);

type Kind = 'api' | 'app' | 'app-library' | 'app-suite' | 'api-suite';
type Visibility = 'PUBLIC' | 'PRIVATE';

interface Target {
    name: string;
    kind: Kind;
    cmake: string;
    dir: string;
    sources: string[];
    links: Array<[string, Visibility]>;
}

function rel(p: string): string {
    return path.relative(ROOT, p).split(path.sep).join('/');
}

function read(p: string): string {
    return readFileSync(p, 'utf-8').replace(/^﻿/, '');
}

function isFile(p: string): boolean {
    return existsSync(p) && statSync(p).isFile();
}

/** The tracked files matching the pathspecs, as plain paths: -z stops git quoting a name. */
function lsFiles(...pathspecs: string[]): string[] {
    return execFileSync('git', ['ls-files', '-z', '--', ...pathspecs], { cwd: ROOT, encoding: 'utf-8', maxBuffer: 64 * 1024 * 1024 })
        .split('\0').filter((line) => line.length > 0);
}

function endsWithAny(name: string, suffixes: string[]): boolean {
    return suffixes.some((s) => name.endsWith(s));
}

// --- CMake ---------------------------------------------------------------------------------

/** Each command call as [name, arguments], with comments and quotes removed. */
function cmakeCommands(text: string): Array<[string, string[]]> {
    const out: Array<[string, string[]]> = [];
    const call = /[A-Za-z_]\w*\s*\(/y;
    let i = 0;
    const n = text.length;
    while (i < n) {
        if (text[i] === '#') {
            const j = text.indexOf('\n', i);
            i = j < 0 ? n : j;
            continue;
        }
        call.lastIndex = i;
        const m = call.exec(text);
        if (!m || (i > 0 && /\w/.test(text[i - 1]))) {
            i += 1;
            continue;
        }
        const name = m[0].replace(/\s*\($/, '');
        i = call.lastIndex;
        let depth = 1;
        let cur = '';
        let quoted = false;
        const args: string[] = [];
        while (i < n && depth > 0) {
            const c = text[i];
            if (quoted) {
                if (c === '\\' && i + 1 < n) {
                    cur += text.slice(i, i + 2);
                    i += 2;
                    continue;
                }
                if (c === '"') {
                    quoted = false;
                    args.push(cur);
                    cur = '';
                } else {
                    cur += c;
                }
            } else if (c === '"') {
                quoted = true;
                cur = '';
            } else if (c === '#') {
                const j = text.indexOf('\n', i);
                i = j < 0 ? n : j;
                continue;
            } else if (' \t\r\n'.includes(c)) {
                if (cur) {
                    args.push(cur);
                }
                cur = '';
            } else if (c === '(') {
                depth += 1;
                cur += c;
            } else if (c === ')') {
                depth -= 1;
                if (depth > 0) {
                    cur += c;
                } else if (cur) {
                    args.push(cur);
                }
            } else {
                cur += c;
            }
            i += 1;
        }
        out.push([name, args]);
    }
    return out;
}

function loadTargets(): { targets: Map<string, Target>; boost: string[] } {
    const files = lsFiles('*CMakeLists.txt', '*.cmake').filter((f) => !f.startsWith('examples/'));
    const targets = new Map<string, Target>();
    const linkCalls: string[][] = [];
    let boost: string[] = [];
    const make = (name: string, kind: Kind, cmake: string, sources: string[]): Target =>
        ({ name, kind, cmake, dir: path.dirname(cmake), sources, links: [] });
    for (const f of files) {
        const file = path.join(ROOT, f);
        const underApi = f.startsWith('api/');
        for (const [name, args] of cmakeCommands(read(file))) {
            const low = name.toLowerCase();
            if (low === 'find_package' && args[0] === 'Boost' && args.includes('COMPONENTS')) {
                boost = args.slice(args.indexOf('COMPONENTS') + 1)
                    .filter((a) => a !== 'REQUIRED' && a !== 'CONFIG').map((a) => 'Boost::' + a);
            }
            if (!f.endsWith('CMakeLists.txt') || args.length === 0) {
                continue;
            }
            const sources = args.slice(1).filter((a) => endsWithAny(a, SOURCE_SUFFIXES))
                .map((a) => path.normalize(path.join(path.dirname(file), a)));
            if (low === 'v3d_add_api_library') {
                targets.set('v3dlib_' + args[0], make('v3dlib_' + args[0], 'api', file, sources));
            } else if (low === 'v3d_add_test') {
                const t = make('v3dtest_' + args[0], underApi ? 'api-suite' : 'app-suite', file, sources);
                t.links.push(['Boost::unit_test_framework', 'PRIVATE']);
                targets.set(t.name, t);
            } else if ((low === 'add_executable' || low === 'add_library') && !args[0].startsWith('${')) {
                if (args.includes('ALIAS') || args.includes('IMPORTED')) {
                    continue;
                }
                targets.set(args[0], make(args[0], low === 'add_executable' ? 'app' : 'app-library', file, sources));
            } else if (low === 'target_link_libraries') {
                linkCalls.push(args);
            }
        }
    }
    for (const args of linkCalls) {
        const t = targets.get(args[0]);
        if (t === undefined) {
            continue;
        }
        let visibility: Visibility = 'PUBLIC';
        for (const a of args.slice(1)) {
            if (LINK_KEYWORDS.has(a)) {
                visibility = a === 'PRIVATE' || a === 'LINK_PRIVATE' ? 'PRIVATE' : 'PUBLIC';
            } else {
                // v3d::<lib> is the alias v3d_add_api_library gives v3dlib_<lib>
                t.links.push([a.replace(/^v3d::(\w+)$/, 'v3dlib_$1'), visibility]);
            }
        }
    }
    for (const t of targets.values()) {
        if (t.kind === 'api') {
            t.links.push(['Boost::headers', 'PUBLIC']);
        }
    }
    return { targets, boost };
}

/** The package targets one link item names. */
function expand(item: string, boost: string[]): string[] {
    const m = /^\$\{(\w+)_LIBRARIES\}$/.exec(item);
    if (!m) {
        return [item];
    }
    if (m[1] === 'Boost') {
        return boost;
    }
    return VARIABLE_TARGETS[m[1]] ?? [item];
}

/** What a consumer of the named target gets from it: the target and its PUBLIC closure. */
function carried(targets: Map<string, Target>, name: string, boost: string[], seen = new Set<string>()): Set<string> {
    const out = new Set<string>();
    if (seen.has(name)) {
        return out;
    }
    seen.add(name);
    out.add(name);
    const t = targets.get(name);
    if (t === undefined) {
        return out;
    }
    for (const [item, visibility] of t.links) {
        if (visibility !== 'PUBLIC') {
            continue;
        }
        for (const x of expand(item, boost)) {
            for (const y of carried(targets, x, boost, seen)) {
                out.add(y);
            }
        }
    }
    return out;
}

// --- C++ -----------------------------------------------------------------------------------

/**
 * Remove comments and #if 0 blocks, and reduce string and character literals to their quotes
 * unless asked to keep them.
 */
function stripCpp(text: string, keepStrings: boolean): string {
    const lines = keepStrings ? blankComments(text).split('\n') : codeLines(text);
    const kept: string[] = [];
    let depth = 0;
    for (const line of lines) {
        const s = line.trim();
        if (depth > 0) {
            if (/^#\s*if/.test(s)) {
                depth += 1;
            } else if (/^#\s*endif/.test(s)) {
                depth -= 1;
            } else if (depth === 1 && /^#\s*(else|elif)/.test(s)) {
                depth = 0;
            }
            kept.push('');
        } else if (/^#\s*if\s+0\b/.test(s)) {
            depth = 1;
            kept.push('');
        } else {
            kept.push(line);
        }
    }
    return kept.join('\n');
}

const INCLUDE = /^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]/gm;
const DECLARATION = /\bnamespace\s+v3d((?:::\w+)+)\s*\{/g;
const FORWARD_ONLY = /\s*(?:(?:class|struct)\s+\w+\s*;\s*)*\}/y;
// A namespace declaration such as "namespace v3d::grid {" names no use, and a using-directive
// such as "using namespace v3d::grid;" does.
const QUALIFIED = /(?<![\w:])(using\s+)?(namespace\s+)?v3d((?:\s*::\s*\w+)+)/g;

interface Scan {
    includes: Array<[string, string | null]>;
    uses: Array<[string[], string]>;
}

class Tree {
    targets: Map<string, Target>;
    cache = new Map<string, Scan>();
    apiPaths = new Map<string, string>();
    appLibraries = new Map<string, string>();
    namespaces = new Map<string, Set<string>>();

    constructor(targets: Map<string, Target>) {
        this.targets = targets;
        const manifest = read(path.join(ROOT, 'cmake', 'v3dApiLibraries.cmake'));
        for (const m of manifest.matchAll(/set\(V3D_API_(\w+?)_PATH\s+"([^"]+)"\)/g)) {
            this.apiPaths.set('v3dlib_' + m[1], 'api/' + m[2] + '/');
        }
        // an app library owns the directory its CMakeLists.txt is in
        for (const t of targets.values()) {
            if (t.kind === 'app-library') {
                this.appLibraries.set(t.name, rel(t.dir) + '/');
            }
        }
        this.findNamespaces();
    }

    /** The target owning a path relative to the root, or null for a local file. */
    owner(relpath: string): string | null {
        if (('/' + relpath).includes('/tests/')) {
            return null;
        }
        let best: string | null = null;
        let length = 0;
        for (const table of [this.apiPaths, this.appLibraries]) {
            for (const [lib, prefix] of table) {
                if (relpath.startsWith(prefix) && prefix.length > length) {
                    best = lib;
                    length = prefix.length;
                }
            }
        }
        return best;
    }

    /** Map each namespace an api library declares, joined by ::, to the libraries declaring it. */
    findNamespaces(): void {
        for (const f of lsFiles('api')) {
            if (!endsWithAny(f, SOURCE_SUFFIXES)) {
                continue;
            }
            const lib = this.owner(f);
            if (lib === null) {
                continue;
            }
            const text = stripCpp(read(path.join(ROOT, f)), false);
            for (const m of text.matchAll(DECLARATION)) {
                // a block of forward declarations names another library's types, not its own
                FORWARD_ONLY.lastIndex = (m.index ?? 0) + m[0].length;
                if (FORWARD_ONLY.test(text)) {
                    continue;
                }
                const key = m[1].split('::').slice(1).join('::');
                if (!this.namespaces.has(key)) {
                    this.namespaces.set(key, new Set());
                }
                this.namespaces.get(key)?.add(lib);
            }
        }
    }

    /** The includes of one file, each with the file it resolves to, and the namespaces it uses. */
    scan(file: string): Scan {
        const cached = this.cache.get(file);
        if (cached !== undefined) {
            return cached;
        }
        const raw = read(file);
        const includes: Array<[string, string | null]> = [];
        for (const name of includesIn(raw)) {
            let found: string | null = null;
            for (const base of [path.dirname(file), ROOT]) {
                const candidate = path.normalize(path.join(base, name));
                if (candidate.startsWith(ROOT) && isFile(candidate)) {
                    found = candidate;
                    break;
                }
            }
            includes.push([name, found]);
        }
        const result = { includes, uses: usesIn(raw, this.namespaces) };
        this.cache.set(file, result);
        return result;
    }
}

/** The text of each include in a C++ file, in order. */
export function includesIn(text: string): string[] {
    return [...stripCpp(text, true).matchAll(INCLUDE)].map((m) => m[1]);
}

/**
 * The names in a v3d::<lib>:: namespace that a C++ file uses, each with the libraries that declare
 * the longest namespace of it found in namespaces.
 */
export function usesIn(text: string, namespaces: Map<string, Set<string>>): Array<[string[], string]> {
    const uses: Array<[string[], string]> = [];
    for (const m of stripCpp(text, false).matchAll(QUALIFIED)) {
        if (m[2] && !m[1]) {
            continue;
        }
        const parts = m[3].split('::').slice(1).map((p) => p.trim());
        for (let k = parts.length; k > 0; k -= 1) {
            const libs = namespaces.get(parts.slice(0, k).join('::'));
            if (libs !== undefined) {
                uses.push([[...libs].sort(), 'v3d::' + parts.join('::')]);
                break;
            }
        }
    }
    return uses;
}

// --- the rules -----------------------------------------------------------------------------

function packageOf(include: string): string | null {
    for (const [target, pattern] of PACKAGE_HEADERS) {
        if (pattern.test(include)) {
            return target;
        }
    }
    return null;
}

type Report = (where: string, rule: string, detail: string) => void;

function check(tree: Tree, t: Target, boost: string[], report: Report): void {
    const cm = rel(t.cmake);
    const own = t.kind === 'app-library' ? rel(t.dir) + '/' : null;

    // walk the target's files
    const queue = t.sources.filter(isFile);
    const seen = new Set<string>();
    const included = new Map<string, [string, string]>();    // v3dlib target -> first include of it
    const headerLibs = new Map<string, [string, string]>();  // for an app library: from a header
    const packages = new Map<string, [string, string]>();    // package target -> first include
    const uses: Array<[string, string[], string]> = [];
    while (queue.length > 0) {
        const f = queue.shift() as string;
        if (seen.has(f)) {
            continue;
        }
        seen.add(f);
        const scan = tree.scan(f);
        for (const [libs, name] of scan.uses) {
            uses.push([f, libs, name]);
        }
        for (const [name, found] of scan.includes) {
            let lib: string | null = null;
            if (name.startsWith('api/') && !name.includes('/tests/')) {
                lib = tree.owner(name);
            } else if (found !== null) {
                const r = rel(found);
                lib = tree.owner(r);
                if (lib !== null && own !== null && r.startsWith(own)) {
                    lib = null;
                }
                if (lib === null) {
                    queue.push(found);
                    continue;
                }
            } else {
                const pkg = packageOf(name);
                if (pkg !== null && !packages.has(pkg)) {
                    packages.set(pkg, [f, name]);
                }
                continue;
            }
            if (lib === null) {
                continue;
            }
            if (!included.has(lib)) {
                included.set(lib, [f, name]);
            }
            if (own !== null && endsWithAny(f, HEADER_SUFFIXES) && !headerLibs.has(lib)) {
                headerLibs.set(lib, [f, name]);
            }
        }
    }

    const named = new Map<string, Visibility>();
    for (const [item, visibility] of t.links) {
        if (!named.has(item)) {
            named.set(item, visibility);
        }
    }
    const namedLibs = [...named.keys()].filter((x) => x.startsWith('v3dlib_')).sort();

    for (const [lib, [f, name]] of [...included].sort()) {
        if (!named.has(lib)) {
            report(cm, 'missing-link', `${rel(f)} includes ${name}, and ${t.name} does not link ${lib}`);
        }
    }
    for (const lib of namedLibs) {
        if (!included.has(lib)) {
            report(cm, 'extra-link', `${t.name} links ${lib}, and none of its files includes its headers`);
        }
    }

    const done = new Set<string>();
    for (const [f, libs, name] of uses) {
        const key = rel(f) + ' ' + libs.join(' ');
        if (libs.some((lib) => included.has(lib)) || done.has(key)) {
            continue;
        }
        done.add(key);
        report(rel(f), 'namespace-use', `names ${name}, and ${t.name} includes no header of ${libs.join(' or ')}`);
    }

    if (t.kind === 'app-library') {
        for (const [lib, [f, name]] of [...headerLibs].sort()) {
            if (named.has(lib) && named.get(lib) !== 'PUBLIC') {
                report(cm, 'visibility', `${rel(f)} includes ${name}, and ${t.name} links ${lib} PRIVATE`);
            }
        }
        for (const lib of namedLibs) {
            if (named.get(lib) === 'PUBLIC' && included.has(lib) && !headerLibs.has(lib)) {
                report(cm, 'visibility', `${t.name} links ${lib} PUBLIC, and none of its headers includes it`);
            }
        }
    }

    if (t.kind !== 'app' && t.kind !== 'app-suite') {
        return;
    }

    // what the v3dlib_* targets this one links carry
    const through = new Set<string>();
    for (const lib of namedLibs) {
        for (const x of carried(tree.targets, lib, boost)) {
            if (x !== lib) {
                through.add(x);
            }
        }
    }
    const suite = t.kind === 'app-suite';
    const direct = new Set<string>();
    for (const item of named.keys()) {
        if (item.startsWith('v3dlib_') || (suite && item === 'Boost::unit_test_framework')) {
            continue;
        }
        const bad: string[] = [];
        for (const pkg of expand(item, boost)) {
            if (suite && pkg === 'Boost::unit_test_framework') {
                bad.push(`v3d_add_test already links ${pkg}`);
            } else if (through.has(pkg)) {
                bad.push(`${pkg} comes through a v3dlib_* target it links`);
            } else if (!packages.has(pkg)) {
                bad.push(`none of its files includes ${pkg}`);
            } else {
                direct.add(pkg);
            }
        }
        if (bad.length > 0) {
            report(cm, 'package', `${t.name} names ${item}: ${bad.join('; ')}`);
        }
    }
    for (const [pkg, [f, name]] of [...packages].sort()) {
        const given = suite && pkg === 'Boost::unit_test_framework';
        if (!through.has(pkg) && !direct.has(pkg) && !given) {
            report(cm, 'package-unlinked', `${rel(f)} includes ${name}, and nothing ${t.name} links carries ${pkg}`);
        }
    }
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    if (process.argv.length > 2) {
        console.error('usage: node scripts/linkrule.ts');
        return 2;
    }
    const { targets, boost } = loadTargets();
    const tree = new Tree(targets);
    const found: string[] = [];
    const report: Report = (where, rule, detail) => {
        found.push(`${where}: ${rule}: ${detail}`);
    };
    for (const name of [...targets.keys()].sort()) {
        const t = targets.get(name) as Target;
        if (t.kind !== 'api') {
            check(tree, t, boost, report);
        }
    }
    for (const line of found) {
        console.log(line);
    }
    return found.length > 0 ? 1 : 0;
}

if (isEntryPoint(import.meta.url)) {
    process.exitCode = main();
}
