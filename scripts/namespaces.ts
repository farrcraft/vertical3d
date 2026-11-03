/**
 * Check that each C++ file's namespace follows its directory, as docs/contributing/Conventions.md
 * sets out under "Language".
 *
 * Each root below gives the namespace of the files directly in it, and a subdirectory adds its
 * own name: api/render/realtime/vulkan/device gives v3d::render::realtime::vulkan::device, and
 * vertical3d/src/scene gives v3d::editor::scene. An internal/ directory is a subdirectory like any
 * other. The script reads every tracked .h, .cpp and .cxx file under a root but those under a
 * tests directory, and reports:
 *
 *   own-namespace      a file that opens a named namespace, and never opens the one its
 *                      directory gives. A file that opens none, such as a main or a header of
 *                      a C interface, is not reported.
 *   no-directory       a namespace under v3d that no checked directory gives, such as
 *                      v3d::render::offline::uniform when there is no uniform directory.
 *
 * A file may open other namespaces besides its own, to forward-declare what another directory
 * defines. The check reads code through scripts/lib/lexer.ts, so nothing in a comment or a literal
 * counts, and it skips preprocessor lines. A nested namespace is named in full, so
 * `namespace v3d::font { namespace internal {` opens both v3d::font and v3d::font::internal.
 *
 * Usage: node scripts/namespaces.ts
 */

import { readFileSync } from 'node:fs';
import * as path from 'node:path';

import { options, report, runGate } from './lib/cli.ts';
import { tracked } from './lib/git.ts';
import { codeLines } from './lib/lexer.ts';
import { ROOT, isTestPath } from './lib/tree.ts';

/** Each root directory, and the namespace of the files directly in it. */
export const ROOTS: Array<[string, string]> = [
    ['api/', 'v3d'],
    ['moya/libmoya/', 'v3d::moya'],
    ['moya/', 'v3d::moya'],
    ['vertical3d/src/', 'v3d::editor'],
];

export type Rule = 'own-namespace' | 'no-directory';

/** One report, and the line it is on. */
export interface Finding {
    line: number;
    rule: Rule;
    message: string;
}

/** A namespace a file opens, and the line its name is on. */
export interface Opened {
    line: number;
    name: string;
}

/**
 * The namespace the directory of a file gives it, or null when the file is under no root or under
 * a tests directory. The first root that holds the file is used, so a deeper root comes first.
 */
export function expected(file: string): string | null {
    if (isTestPath(file)) {
        return null;
    }
    const root = ROOTS.find(([dir]) => file.startsWith(dir));
    if (root === undefined) {
        return null;
    }
    const dirs = file.slice(root[0].length).split('/').slice(0, -1);
    return [root[1], ...dirs].join('::');
}

/** Every named namespace a C++ text opens, in full, with the line each opens on. */
export function opened(text: string): Opened[] {
    // preprocessor lines are blanked, so a macro body holds no namespace
    let continued = false;
    const lines = codeLines(text).map((code) => {
        const preprocessor = continued || code.trimStart().startsWith('#');
        continued = preprocessor && code.trimEnd().endsWith('\\');
        return preprocessor ? '' : code;
    });
    const code = lines.join('\n');
    const result: Opened[] = [];
    // null for a brace that opens no named namespace, and '' for an anonymous namespace
    const stack: Array<string | null> = [];
    const pattern = /\b(?:inline\s+)?namespace\s+((?:\w+\s*::\s*)*\w+)?\s*\{|\{|\}/g;
    for (const match of code.matchAll(pattern)) {
        if (match[0] === '{') {
            stack.push(null);
        } else if (match[0] === '}') {
            stack.pop();
        } else if (match[1] === undefined) {
            stack.push('');
        } else {
            stack.push(match[1].replace(/\s+/g, ''));
            const name = stack.filter((s) => s !== null && s !== '').join('::');
            const line = code.slice(0, match.index).split('\n').length;
            result.push({ line, name });
        }
    }
    return result;
}

/**
 * The findings for one file, given the namespace its directory gives it and every namespace a
 * checked directory gives.
 */
export function findings(text: string, own: string, known: Set<string>): Finding[] {
    const found: Finding[] = [];
    const names = opened(text);
    for (const { line, name } of names) {
        if ((name === 'v3d' || name.startsWith('v3d::')) && !known.has(name)) {
            found.push({ line, rule: 'no-directory', message: `${name} is the namespace of no directory` });
        }
    }
    if (names.length > 0 && !names.some(({ name }) => name === own)) {
        found.push({ line: names[0].line, rule: 'own-namespace', message: `the directory gives ${own}, and the file never opens it` });
    }
    return found;
}

/** The tracked C++ files the check reads, relative to the root, each with its namespace. */
export function files(): Array<[string, string]> {
    return tracked('*.h', '*.cpp', '*.cxx')
        .flatMap((f) => {
            const own = expected(f);
            return own === null ? [] : [[f, own] as [string, string]];
        });
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/namespaces.ts', {});
    const checked = files();
    const known = new Set(checked.map(([, own]) => own));
    const found = checked.flatMap(([file, own]) => findings(readFileSync(path.join(ROOT, file), 'utf8'), own, known)
        .map(({ line, rule, message }) => ({ path: file, line, rule, message })));
    return report(found, 'puts code in a namespace other than its directory\'s, which docs/contributing/Conventions.md does not allow.');
}

runGate(import.meta.url, main);
