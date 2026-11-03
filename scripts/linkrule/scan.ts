/**
 * The tree the link rule walks: which target owns each path, which libraries declare each
 * namespace, and what each file includes and uses.
 */

import * as path from 'node:path';

import { isTestPath } from '../lib/tree.ts';
import { DECLARATION, FORWARD_ONLY, includesIn, stripCpp, usesIn } from './cpp.ts';
import { SOURCE_SUFFIXES, endsWithAny, isFile, read, rel } from './files.ts';
import type { Target } from './targets.ts';

export interface Scan {
    includes: Array<[string, string | null]>;
    uses: Array<[string[], string]>;
}

export class Tree {
    root: string;
    apiFiles: readonly string[];
    targets: Map<string, Target>;
    cache = new Map<string, Scan>();
    apiPaths = new Map<string, string>();
    appLibraries = new Map<string, string>();
    namespaces = new Map<string, Set<string>>();

    constructor(root: string, targets: Map<string, Target>, apiFiles: readonly string[]) {
        this.root = root;
        this.apiFiles = apiFiles;
        this.targets = targets;
        const manifest = read(path.join(root, 'cmake', 'v3dApiLibraries.cmake'));
        for (const m of manifest.matchAll(/set\(V3D_API_(\w+?)_PATH\s+"([^"]+)"\)/g)) {
            this.apiPaths.set('v3dlib_' + m[1], 'api/' + m[2] + '/');
        }
        // an app library owns the directory its CMakeLists.txt is in
        for (const t of targets.values()) {
            if (t.kind === 'app-library') {
                this.appLibraries.set(t.name, rel(root, t.dir) + '/');
            }
        }
        this.findNamespaces();
    }

    /** The target owning a path relative to the root, or null for a local file. */
    owner(relpath: string): string | null {
        if (isTestPath(relpath)) {
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
        for (const f of this.apiFiles) {
            if (!endsWithAny(f, SOURCE_SUFFIXES)) {
                continue;
            }
            const lib = this.owner(f);
            if (lib === null) {
                continue;
            }
            const text = stripCpp(read(path.join(this.root, f)), false);
            for (const m of text.matchAll(DECLARATION)) {
                // a block of forward declarations names another library's types, not its own
                FORWARD_ONLY.lastIndex = (m.index ?? 0) + m[0].length;
                if (FORWARD_ONLY.test(text)) {
                    continue;
                }
                const key = (m[1] ?? '').split('::').slice(1).join('::');
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
            for (const base of [path.dirname(file), this.root]) {
                const candidate = path.normalize(path.join(base, name));
                if (candidate.startsWith(this.root) && isFile(candidate)) {
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
