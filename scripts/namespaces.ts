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
 *                      v3d::rib::uniform when there is no uniform directory.
 *   no-description     a namespace of the api reference that nothing describes.
 *   described-twice    a namespace described more than once.
 *
 * The namespaces of the api reference are those of the public headers under api/, the headers
 * outside a tests or internal directory, and every namespace above one of them. A namespace is
 * described by a doc comment directly above a line that opens it, in a public header, or by an
 * @namespace block in scripts/docs/namespaces.dox, which describes a namespace whose directory
 * holds no header.
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

/** The file that describes the namespaces whose directories hold no header. */
export const NAMESPACES_DOX = 'scripts/docs/namespaces.dox';

export type Rule = 'own-namespace' | 'no-directory' | 'no-description' | 'described-twice';

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
    const [first] = names;
    if (first !== undefined && !names.some(({ name }) => name === own)) {
        found.push({ line: first.line, rule: 'own-namespace', message: `the directory gives ${own}, and the file never opens it` });
    }
    return found;
}

/**
 * Every namespace a header describes, with the line its name is on. A doc comment directly above
 * the line that opens a namespace describes it.
 */
export function described(text: string): Opened[] {
    const result: Opened[] = [];
    for (const match of text.matchAll(/\/\*\*(?:[^*]|\*(?!\/))*\*\/[ \t]*\r?\n[ \t]*namespace\s+((?:\w+\s*::\s*)*\w+)\s*\{/g)) {
        const name = (match[1] ?? '').replace(/\s+/g, '');
        const line = text.slice(0, match.index + match[0].lastIndexOf('namespace')).split('\n').length;
        result.push({ line, name });
    }
    return result;
}

/** Every namespace an @namespace block describes, with the line it is named on. */
export function declared(text: string): Opened[] {
    return text.split('\n').flatMap((code, index) => {
        const match = /@namespace\s+([\w:]+)/.exec(code);
        return match === null ? [] : [{ line: index + 1, name: match[1] ?? '' }];
    });
}

/** One public header: its path, its namespace and its text. */
export interface Header {
    file: string;
    own: string;
    text: string;
}

/** A finding of descriptionFindings, which names its file because it may be any header's. */
export interface Placed extends Finding {
    path: string;
}

/**
 * The findings for the descriptions of the api reference's namespaces, given the public headers
 * and the text of scripts/docs/namespaces.dox. A namespace that nothing describes is reported on
 * the first header that opens it, or on namespaces.dox when no header does.
 */
export function descriptionFindings(headers: readonly Header[], dox: string): Placed[] {
    const missing = new Map<string, Placed>();
    const places = new Map<string, Array<{ path: string; line: number }>>();
    const add = (name: string, place: { path: string; line: number }): void => {
        places.set(name, [...(places.get(name) ?? []), place]);
    };
    for (const { file, own, text } of [...headers].toSorted((a, b) => a.file.localeCompare(b.file))) {
        const opening = opened(text).find(({ name }) => name === own);
        if (!missing.has(own) && opening !== undefined) {
            missing.set(own, { path: file, line: opening.line, rule: 'no-description', message: `nothing describes ${own}` });
        }
        for (const { line, name } of described(text)) {
            add(name, { path: file, line });
        }
    }
    for (const { line, name } of declared(dox)) {
        add(name, { path: NAMESPACES_DOX, line });
    }
    // the loop adds parents to missing, so it walks a copy of the names found before it
    for (const own of Array.from(missing.keys())) {
        const parts = own.split('::');
        for (let length = 1; length < parts.length; length += 1) {
            const parent = parts.slice(0, length).join('::');
            if (!missing.has(parent)) {
                missing.set(parent, { path: NAMESPACES_DOX, line: 1, rule: 'no-description', message: `nothing describes ${parent}` });
            }
        }
    }
    const found: Placed[] = [];
    for (const [name, finding] of missing) {
        const [earlier, ...later] = places.get(name) ?? [];
        if (earlier === undefined) {
            found.push(finding);
        }
        for (const place of later) {
            found.push({
                ...place,
                rule: 'described-twice',
                message: `${name} is described already, at ${earlier?.path}:${earlier?.line}`,
            });
        }
    }
    return found;
}

/** The tracked C++ files the check reads, relative to the root, each with its namespace. */
export function files(): Array<[string, string]> {
    return tracked('*.h', '*.cpp', '*.cxx').flatMap((f) => {
        const own = expected(f);
        return own === null ? [] : [[f, own] as [string, string]];
    });
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/namespaces.ts', {});
    const checked = files();
    const known = new Set(checked.map(([, own]) => own));
    const texts = new Map(checked.map(([file]) => [file, readFileSync(path.join(ROOT, file), 'utf8')]));
    const found = checked.flatMap(([file, own]) =>
        findings(texts.get(file) ?? '', own, known).map(({ line, rule, message }) => ({ path: file, line, rule, message })),
    );
    const headers = checked
        .filter(([file]) => file.startsWith('api/') && file.endsWith('.h') && !file.includes('/internal/'))
        .map(([file, own]) => ({ file, own, text: texts.get(file) ?? '' }));
    found.push(...descriptionFindings(headers, readFileSync(path.join(ROOT, NAMESPACES_DOX), 'utf8')));
    return report(found, 'breaks a rule for namespaces in docs/contributing/Conventions.md.');
}

runGate(import.meta.url, main);
