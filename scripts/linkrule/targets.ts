/**
 * The targets a tree's CMake files define, what each links, and what a link carries to the
 * target that names it.
 */

import * as path from 'node:path';

import { SOURCE_SUFFIXES, endsWithAny, read } from './files.ts';

// A package variable such as ${PNG_LIBRARIES} names the target of the package it is named after.
const VARIABLE_TARGETS: Record<string, string[]> = {
    PNG: ['PNG::PNG'],
    JPEG: ['JPEG::JPEG'],
    SDL3: ['SDL3::SDL3'],
    Vulkan: ['Vulkan::Vulkan'],
    Freetype: ['Freetype::Freetype'],
};

const LINK_KEYWORDS = new Set(['PUBLIC', 'PRIVATE', 'INTERFACE', 'LINK_PUBLIC', 'LINK_PRIVATE']);

export type Kind = 'api' | 'app' | 'app-library' | 'app-suite' | 'api-suite';

export type Visibility = 'PUBLIC' | 'PRIVATE';

export interface Target {
    name: string;
    kind: Kind;
    cmake: string;
    dir: string;
    sources: string[];
    links: Array<[string, Visibility]>;
}

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
        if (!m || (i > 0 && /\w/.test(text.charAt(i - 1)))) {
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
            const c = text.charAt(i);
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

/** The targets the CMake files of a tree define, and the Boost components it finds. */
export function loadTargets(root: string, files: readonly string[]): { targets: Map<string, Target>; boost: string[] } {
    const targets = new Map<string, Target>();
    const linkCalls: string[][] = [];
    let boost: string[] = [];
    const make = (name: string, kind: Kind, cmake: string, sources: string[]): Target =>
        ({ name, kind, cmake, dir: path.dirname(cmake), sources, links: [] });
    for (const f of files) {
        const file = path.join(root, f);
        const underApi = f.startsWith('api/');
        for (const [name, args] of cmakeCommands(read(file))) {
            const low = name.toLowerCase();
            if (low === 'find_package' && args[0] === 'Boost' && args.includes('COMPONENTS')) {
                boost = args.slice(args.indexOf('COMPONENTS') + 1)
                    .filter((a) => a !== 'REQUIRED' && a !== 'CONFIG').map((a) => 'Boost::' + a);
            }
            const [first] = args;
            if (!f.endsWith('CMakeLists.txt') || first === undefined) {
                continue;
            }
            const sources = args.slice(1).filter((a) => endsWithAny(a, SOURCE_SUFFIXES))
                .map((a) => path.normalize(path.join(path.dirname(file), a)));
            if (low === 'v3d_add_api_library') {
                targets.set('v3dlib_' + first, make('v3dlib_' + first, 'api', file, sources));
            } else if (low === 'v3d_add_test') {
                // v3d_add_test links the test framework, which the package rule takes as given
                const t = make('v3dtest_' + first, underApi ? 'api-suite' : 'app-suite', file, sources);
                targets.set(t.name, t);
            } else if ((low === 'add_executable' || low === 'add_library') && !first.startsWith('${')) {
                if (args.includes('ALIAS') || args.includes('IMPORTED')) {
                    continue;
                }
                targets.set(first, make(first, low === 'add_executable' ? 'app' : 'app-library', file, sources));
            } else if (low === 'target_link_libraries') {
                linkCalls.push(args);
            }
        }
    }
    for (const [name = '', ...items] of linkCalls) {
        const t = targets.get(name);
        if (t === undefined) {
            continue;
        }
        let visibility: Visibility = 'PUBLIC';
        for (const a of items) {
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
export function expand(item: string, boost: string[]): string[] {
    const variable = /^\$\{(\w+)_LIBRARIES\}$/.exec(item)?.[1];
    if (variable === undefined) {
        return [item];
    }
    if (variable === 'Boost') {
        return boost;
    }
    return VARIABLE_TARGETS[variable] ?? [item];
}

/** What a consumer of the named target gets from it: the target and its PUBLIC closure. */
export function carried(targets: Map<string, Target>, name: string, boost: string[], seen = new Set<string>()): Set<string> {
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
