/**
 * Checks moya's kernel code for what a GPU compiler rejects, as docs/offline/Kernel.md sets out.
 *
 * Kernel code is every C++ file under moya/libmoya/kernel/. The CPU compiles it with exceptions
 * and RTTI off, which catches some of what a GPU cannot run. This script reports the rest, from
 * the source, so a breach fails before any GPU build exists:
 *
 *   virtual      a virtual function. A GPU has no virtual call.
 *   allocation   new, delete, malloc and its kin, or a standard or boost type that allocates.
 *                A deleted function, = delete, is not an allocation.
 *   exception    throw, try or catch.
 *   rtti         dynamic_cast or typeid.
 *   include      an include of anything but a kernel header, a glm header, or one of the
 *                standard headers every device compiler provides: cfloat, climits, cmath,
 *                cstddef, cstdint, limits and type_traits.
 *   uncompiled   a kernel header that no source of the kernel target includes, so the kernel's
 *                own rules never compile it.
 *
 * Comments and the insides of literals are not read, except that an include's path is read from
 * its line.
 *
 * Usage: node scripts/kernel.ts
 *
 * Each report is printed as path:line: rule: excerpt, or path: rule: excerpt for a whole file.
 * The exit status is 1 when there is any report, and 0 otherwise.
 *
 * Node runs this file directly: it uses only node: modules, the gates' modules in scripts/lib,
 * and type annotations Node can strip.
 */

import { existsSync, readFileSync } from 'node:fs';
import * as path from 'node:path';

import { type Finding, options, report, runGate } from './lib/cli.ts';
import { tracked, untracked } from './lib/git.ts';
import { codeLines } from './lib/lexer.ts';
import { ROOT } from './lib/tree.ts';

/** The directory that holds kernel code. */
export const KERNEL = 'moya/libmoya/kernel/';

const SOURCE = /\.(h|cxx|cpp)$/;

/** The standard headers kernel code may include. */
const STANDARD = new Set(['cfloat', 'climits', 'cmath', 'cstddef', 'cstdint', 'limits', 'type_traits']);

const RULES: Array<[string, RegExp]> = [
    ['virtual', /\bvirtual\b/],
    [
        'allocation',
        new RegExp(
            [
                '\\bnew\\b',
                '(?<!=\\s*)\\bdelete\\b',
                '\\b(?:malloc|calloc|realloc|free|alloca)\\s*\\(',
                '\\bstd::(?:vector|string|wstring|basic_string|map|multimap|unordered_map|set|multiset|unordered_set|list|forward_list|deque|function|shared_ptr|unique_ptr|make_shared|make_unique)\\b',
                '\\bboost::',
            ].join('|'),
        ),
    ],
    ['exception', /\b(?:throw|try|catch)\b/],
    ['rtti', /\b(?:dynamic_cast|typeid)\b/],
];

const INCLUDE = /^\s*#\s*include\s*([<"])([^>"]+)[>"]/;

/** The path an include names, or null for a line that is not an include. */
export function included(text: string): string | null {
    return INCLUDE.exec(text)?.[2] ?? null;
}

/** Whether kernel code may include the path an include names. */
export function allowed(header: string): boolean {
    return header.startsWith(KERNEL) || header.startsWith('glm/') || STANDARD.has(header);
}

/** The findings in one kernel file, from its text. */
export function findings(file: string, text: string): Finding[] {
    const found: Finding[] = [];
    const raw = text.split(/\r?\n/);
    codeLines(text).forEach((code, index) => {
        const line = index + 1;
        const source = raw[index] ?? '';
        // an include's path is a literal to the lexer, so it is read from the line itself
        if (/^\s*#\s*include\b/.test(code)) {
            const header = included(source);
            if (header !== null && !allowed(header)) {
                found.push({ path: file, line, rule: 'include', message: source.trim() });
            }
            return;
        }
        for (const [rule, pattern] of RULES) {
            if (pattern.test(code)) {
                found.push({ path: file, line, rule, message: source.trim() });
            }
        }
    });
    return found;
}

/**
 * The kernel headers that no kernel source includes, from the text of each kernel file. A
 * header included only by another header is not compiled under the kernel's rules unless a
 * source includes it.
 */
export function uncompiled(files: ReadonlyMap<string, string>): Finding[] {
    const reached = new Set<string>();
    for (const [file, text] of files) {
        if (!file.endsWith('.h')) {
            const raw = text.split(/\r?\n/);
            codeLines(text).forEach((code, index) => {
                const header = /^\s*#\s*include\b/.test(code) ? included(raw[index] ?? '') : null;
                if (header !== null) {
                    reached.add(header);
                }
            });
        }
    }
    return [...files.keys()]
        .filter((file) => file.endsWith('.h') && !reached.has(file))
        .map((file) => ({ path: file, line: null, rule: 'uncompiled', message: 'no source of the kernel target includes it' }));
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/kernel.ts', {});
    const kernel = [...tracked(KERNEL), ...untracked().filter((file) => file.startsWith(KERNEL))]
        // a tracked file deleted in the working tree has nothing left to check
        .filter((file) => SOURCE.test(file) && existsSync(path.join(ROOT, file)))
        .toSorted();
    const files = new Map(kernel.map((file) => [file, readFileSync(path.join(ROOT, file), 'utf8')]));
    const found = [...files].flatMap(([file, text]) => findings(file, text));
    found.push(...uncompiled(files));
    return report(found, 'is something kernel code may not do, as docs/offline/Kernel.md sets out.');
}

runGate(import.meta.url, main);
