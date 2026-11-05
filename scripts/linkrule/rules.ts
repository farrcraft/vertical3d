/**
 * The rules the link rule checks on each app, app library and test suite.
 */

import { isTestPath } from '../lib/tree.ts';
import { packageOf } from './cpp.ts';
import { HEADER_SUFFIXES, endsWithAny, isFile, rel } from './files.ts';
import type { Tree } from './scan.ts';
import { type Target, type Visibility, carried, expand } from './targets.ts';

export type Report = (where: string, rule: string, detail: string) => void;

/** A file and the text of one of its includes. */
type Include = [string, string];

/** What a target's files include and use, found by following its local includes. */
interface Walk {
    /** Each v3dlib_* target the files include, with its first include. */
    included: Map<string, Include>;
    /** For an app library: each v3dlib_* target one of its public headers includes. */
    headerLibs: Map<string, Include>;
    /** Each package target the files include, with its first include. */
    packages: Map<string, Include>;
    /** Each name in a v3d namespace a file uses: the file, the libraries declaring it, the name. */
    uses: Array<[string, string[], string]>;
}

/**
 * Reads a target's files: its sources, and every header they include that no library owns,
 * followed transitively. own is the directory of an app library, whose headers are its own.
 */
function walk(tree: Tree, t: Target, own: string | null): Walk {
    const queue = t.sources.filter(isFile);
    const seen = new Set<string>();
    const result: Walk = { included: new Map(), headerLibs: new Map(), packages: new Map(), uses: [] };
    while (queue.length > 0) {
        const f = queue.shift() as string;
        if (seen.has(f)) {
            continue;
        }
        seen.add(f);
        const scan = tree.scan(f);
        for (const [libs, name] of scan.uses) {
            result.uses.push([f, libs, name]);
        }
        for (const [name, found] of scan.includes) {
            let lib: string | null = null;
            if (name.startsWith('api/') && !isTestPath(name)) {
                lib = tree.owner(name);
            } else if (found !== null) {
                const r = rel(tree.root, found);
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
                if (pkg !== null && !result.packages.has(pkg)) {
                    result.packages.set(pkg, [f, name]);
                }
                continue;
            }
            if (lib === null) {
                continue;
            }
            if (!result.included.has(lib)) {
                result.included.set(lib, [f, name]);
            }
            // a header under internal/ is included only by the library's own sources, so what it
            // includes is linked PRIVATE
            if (own !== null && endsWithAny(f, HEADER_SUFFIXES) && !rel(tree.root, f).includes('/internal/') && !result.headerLibs.has(lib)) {
                result.headerLibs.set(lib, [f, name]);
            }
        }
    }
    return result;
}

/** Each item a target links, with the visibility it is first named with. */
function namedLinks(t: Target): Map<string, Visibility> {
    const named = new Map<string, Visibility>();
    for (const [item, visibility] of t.links) {
        if (!named.has(item)) {
            named.set(item, visibility);
        }
    }
    return named;
}

/** missing-link and extra-link: the target links exactly the libraries its files include. */
function checkLinks(tree: Tree, t: Target, found: Walk, named: Map<string, Visibility>, report: Report): void {
    const cm = rel(tree.root, t.cmake);
    for (const [lib, [f, name]] of [...found.included].sort()) {
        if (!named.has(lib)) {
            report(cm, 'missing-link', `${rel(tree.root, f)} includes ${name}, and ${t.name} does not link ${lib}`);
        }
    }
    for (const lib of [...named.keys()].filter((x) => x.startsWith('v3dlib_')).sort()) {
        if (!found.included.has(lib)) {
            report(cm, 'extra-link', `${t.name} links ${lib}, and none of its files includes its headers`);
        }
    }
}

/** namespace-use: a file that names a library's namespace belongs to a target that includes it. */
function checkNamespaces(tree: Tree, t: Target, found: Walk, report: Report): void {
    const done = new Set<string>();
    for (const [f, libs, name] of found.uses) {
        const key = rel(tree.root, f) + ' ' + libs.join(' ');
        if (libs.some((lib) => found.included.has(lib)) || done.has(key)) {
            continue;
        }
        done.add(key);
        report(rel(tree.root, f), 'namespace-use', `names ${name}, and ${t.name} includes no header of ${libs.join(' or ')}`);
    }
}

/** visibility: an app library links PUBLIC exactly what its headers include. */
function checkVisibility(tree: Tree, t: Target, found: Walk, named: Map<string, Visibility>, report: Report): void {
    const cm = rel(tree.root, t.cmake);
    for (const [lib, [f, name]] of [...found.headerLibs].sort()) {
        if (named.has(lib) && named.get(lib) !== 'PUBLIC') {
            report(cm, 'visibility', `${rel(tree.root, f)} includes ${name}, and ${t.name} links ${lib} PRIVATE`);
        }
    }
    for (const lib of [...named.keys()].filter((x) => x.startsWith('v3dlib_')).sort()) {
        if (named.get(lib) === 'PUBLIC' && found.included.has(lib) && !found.headerLibs.has(lib)) {
            report(cm, 'visibility', `${t.name} links ${lib} PUBLIC, and none of its headers includes it`);
        }
    }
}

/**
 * package and package-unlinked: an app or an app suite names a package only when its own files
 * include it and no library it links carries it, and every package it includes reaches it.
 */
function checkPackages(tree: Tree, t: Target, found: Walk, named: Map<string, Visibility>, boost: string[], report: Report): void {
    const cm = rel(tree.root, t.cmake);
    // what the v3dlib_* targets this one links carry
    const through = new Set<string>();
    for (const lib of [...named.keys()].filter((x) => x.startsWith('v3dlib_')).sort()) {
        for (const x of carried(tree.targets, lib, boost)) {
            if (x !== lib) {
                through.add(x);
            }
        }
    }
    const suite = t.kind === 'app-suite';
    const direct = new Set<string>();
    for (const item of named.keys()) {
        if (item.startsWith('v3dlib_')) {
            continue;
        }
        const bad: string[] = [];
        for (const pkg of expand(item, boost)) {
            if (suite && pkg === 'Boost::unit_test_framework') {
                bad.push(`v3d_add_test already links ${pkg}`);
            } else if (through.has(pkg)) {
                bad.push(`${pkg} comes through a v3dlib_* target it links`);
            } else if (!found.packages.has(pkg)) {
                bad.push(`none of its files includes ${pkg}`);
            } else {
                direct.add(pkg);
            }
        }
        if (bad.length > 0) {
            report(cm, 'package', `${t.name} names ${item}: ${bad.join('; ')}`);
        }
    }
    for (const [pkg, [f, name]] of [...found.packages].sort()) {
        const given = suite && pkg === 'Boost::unit_test_framework';
        if (!through.has(pkg) && !direct.has(pkg) && !given) {
            report(cm, 'package-unlinked', `${rel(tree.root, f)} includes ${name}, and nothing ${t.name} links carries ${pkg}`);
        }
    }
}

/** Reports every rule a target breaks. */
export function check(tree: Tree, t: Target, boost: string[], report: Report): void {
    const own = t.kind === 'app-library' ? rel(tree.root, t.dir) + '/' : null;
    const found = walk(tree, t, own);
    const named = namedLinks(t);
    checkLinks(tree, t, found, named, report);
    checkNamespaces(tree, t, found, report);
    if (t.kind === 'app-library') {
        checkVisibility(tree, t, found, named, report);
    }
    if (t.kind === 'app' || t.kind === 'app-suite') {
        checkPackages(tree, t, found, named, boost, report);
    }
}
