/**
 * Check that each component of the tree has a README.md, and that each README is sound.
 *
 * The components are the tracked directories at the repository root, except those whose names
 * start with a dot. Each directory under demos/ or examples/ that holds a CMakeLists.txt is one
 * too, and so is each api library's directory, as cmake/v3dApiLibraries.cmake names it. The check
 * reports:
 *
 *   missing            a component with no README.md.
 *   no-heading         a README whose first line is not a "# " heading.
 *   broken-link        a relative link to a path that does not exist. The part after a "#" is
 *                      not checked.
 *   stale-description  an api library's README whose first paragraph after the heading is not
 *                      the library's description in the table of docs/api/README.md. The two are
 *                      compared with each link read as its text, and each run of whitespace read
 *                      as one space, so the README can link the same page from its own directory.
 *
 * Usage: node scripts/readmes.ts
 */

import { existsSync, readFileSync } from 'node:fs';
import * as path from 'node:path';

import { type Finding, options, report, runGate } from './lib/cli.ts';
import { tracked, untracked } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';
import { type Library, manifestLibraries, tableRows } from './librarytable.ts';

const MANIFEST = 'cmake/v3dApiLibraries.cmake';
const TABLE = 'docs/api/README.md';

/** The directories whose subdirectories with a CMakeLists.txt are components too. */
const PROJECT_PARENTS: readonly string[] = ['demos', 'examples'];

/** Every component directory, relative to the root, sorted and without repeats. */
export function components(files: readonly string[], libraries: readonly Library[]): string[] {
    const found = new Set<string>();
    for (const file of files) {
        const parts = file.split('/');
        const top = parts[0] ?? '';
        if (parts.length > 1 && !top.startsWith('.')) {
            found.add(top);
        }
        if (parts.length === 3 && PROJECT_PARENTS.includes(top) && parts[2] === 'CMakeLists.txt') {
            found.add(`${top}/${parts[1] ?? ''}`);
        }
    }
    for (const library of libraries) {
        found.add(`api/${library.directory}`);
    }
    return [...found].sort();
}

/** Markdown text with each link replaced by its text, and each run of whitespace made one space. */
export function plain(text: string): string {
    return text.replace(/\[([^\]]*)\]\([^)]*\)/g, '$1').replace(/\s+/g, ' ').trim();
}

/** The first paragraph after the heading, as plain() gives it. */
export function firstParagraph(text: string): string {
    const lines = text.split(/\r?\n/).slice(1);
    const start = lines.findIndex((line) => line.trim() !== '');
    if (start < 0) {
        return '';
    }
    const end = lines.findIndex((line, index) => index > start && line.trim() === '');
    return plain(lines.slice(start, end < 0 ? undefined : end).join(' '));
}

/** The relative link targets in a Markdown text, without their "#" parts. */
export function relativeLinks(text: string): string[] {
    const targets: string[] = [];
    for (const match of text.matchAll(/\]\(([^)\s]+)\)/g)) {
        const target = match[1] ?? '';
        if (/^[a-z][a-z0-9+.-]*:/i.test(target) || target.startsWith('#')) {
            continue;
        }
        targets.push(target.split('#')[0] ?? '');
    }
    return targets;
}

/**
 * What is wrong with one component's README. text is null when the README does not exist.
 * description is the library's table description when the component is an api library.
 * exists says whether a path relative to the root exists.
 */
export function problems(directory: string, text: string | null, description: string | undefined,
    exists: (file: string) => boolean): Finding[] {
    const file = `${directory}/README.md`;
    if (text === null) {
        return [{ path: file, line: null, rule: 'missing', message: `${directory} has no README.md` }];
    }
    const found: Finding[] = [];
    if (!/^# \S/.test(text)) {
        found.push({ path: file, line: 1, rule: 'no-heading', message: 'the first line is not a "# " heading' });
    }
    for (const target of relativeLinks(text)) {
        const resolved = path.posix.normalize(path.posix.join(directory, target)).replace(/\/$/, '');
        if (!exists(resolved)) {
            found.push({ path: file, line: null, rule: 'broken-link', message: `${target} does not exist` });
        }
    }
    if (description !== undefined) {
        const wanted = plain(description);
        if (firstParagraph(text) !== wanted) {
            found.push({ path: file, line: null, rule: 'stale-description',
                message: `the first paragraph is not the description in ${TABLE}: "${wanted}"` });
        }
    }
    return found;
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/readmes.ts', {});
    const libraries = manifestLibraries(readFileSync(path.join(ROOT, MANIFEST), 'utf8'));
    const descriptions = new Map<string, string>();
    const rows = tableRows(readFileSync(path.join(ROOT, TABLE), 'utf8'));
    for (const library of libraries) {
        const row = rows.find((candidate) => candidate.name === library.name);
        if (row !== undefined) {
            descriptions.set(`api/${library.directory}`, row.text);
        }
    }
    const exists = (file: string): boolean => existsSync(path.join(ROOT, file));
    const found: Finding[] = [];
    for (const directory of components([...tracked(), ...untracked()], libraries)) {
        const file = path.join(ROOT, directory, 'README.md');
        const text = existsSync(file) ? readFileSync(file, 'utf8') : null;
        found.push(...problems(directory, text, descriptions.get(directory), exists));
    }
    return report(found, 'breaks the rule in docs/contributing/Conventions.md, under "READMEs".');
}

runGate(import.meta.url, main);
