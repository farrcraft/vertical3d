/**
 * Check the table of libraries in docs/api/README.md against the manifest in
 * cmake/v3dApiLibraries.cmake.
 *
 * The manifest's V3D_API_LIBRARIES names every api library, and V3D_API_<name>_PATH the
 * directory under api/ that holds it. The table under the "## The libraries" heading has a row
 * for each, whose first cell links the library's name to that directory.
 *
 * The check reports a library with no row, a row for a name that is not a library, and a library
 * with two rows. It also reports a row that links somewhere other than its library's directory,
 * and rows out of the manifest's order.
 *
 * Usage: node scripts/librarytable.ts
 */

import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { shouldRun } from './entry.ts';

const ROOT = path.resolve(import.meta.dirname, '..');
const MANIFEST = 'cmake/v3dApiLibraries.cmake';
const TABLE = 'docs/api/README.md';
const HEADING = '## The libraries';

/** One api library: its name after v3dlib_, and its directory under api/. */
export interface Library {
    name: string;
    directory: string;
}

/** One row of the table: the name its first cell gives, where that links, and its line. */
export interface Row {
    name: string;
    link: string;
    line: number;
}

/** Every library the manifest names, in its order, with the directory it is in. */
export function manifestLibraries(cmake: string): Library[] {
    const code = cmake.split(/\r?\n/).map((line) => line.replace(/#.*$/, '')).join('\n');
    const list = /set\(\s*V3D_API_LIBRARIES\b([^)]*)\)/.exec(code);
    if (list === null) {
        return [];
    }
    const names = list[1].split(/\s+/).filter((name) => name !== '');
    return names.map((name) => {
        const set = new RegExp(`set\\(\\s*V3D_API_${name}_PATH\\s+"([^"]*)"\\s*\\)`).exec(code);
        return { name, directory: set === null ? '' : set[1] };
    });
}

/** The rows of the first table after the heading, up to the next heading. */
export function tableRows(markdown: string): Row[] {
    const lines = markdown.split(/\r?\n/);
    const start = lines.findIndex((line) => line.trim() === HEADING);
    if (start < 0) {
        return [];
    }
    const rows: Row[] = [];
    for (let index = start + 1; index < lines.length; index++) {
        const line = lines[index];
        if (line.startsWith('#')) {
            break;
        }
        const cell = /^\|\s*\[`([^`]+)`\]\(([^)]*)\)\s*\|/.exec(line);
        if (cell !== null) {
            rows.push({ name: cell[1], link: cell[2], line: index + 1 });
        }
    }
    return rows;
}

/** What is wrong with the table, one message a problem. */
export function problems(libraries: Library[], rows: Row[]): string[] {
    const found: string[] = [];
    if (libraries.length === 0) {
        return [`${MANIFEST} names no libraries in V3D_API_LIBRARIES`];
    }
    const known = new Map(libraries.map((library) => [library.name, library]));
    const seen = new Set<string>();
    for (const row of rows) {
        const library = known.get(row.name);
        if (library === undefined) {
            found.push(`${TABLE}:${row.line}: ${row.name} is not a library ${MANIFEST} names`);
            continue;
        }
        if (seen.has(row.name)) {
            found.push(`${TABLE}:${row.line}: ${row.name} has a row already`);
            continue;
        }
        seen.add(row.name);
        const wanted = `../../api/${library.directory}/`;
        if (row.link !== wanted) {
            found.push(`${TABLE}:${row.line}: ${row.name} links ${row.link}, and its directory is ${wanted}`);
        }
    }
    for (const library of libraries) {
        if (!seen.has(library.name)) {
            found.push(`${TABLE}: ${library.name} has no row`);
        }
    }
    // the order is checked only once the rows and the libraries are the same set, so a missing
    // or extra row is not reported a second time as a row out of place
    if (found.length === 0) {
        const order = libraries.map((library) => library.name);
        rows.forEach((row, index) => {
            if (row.name !== order[index]) {
                found.push(`${TABLE}:${row.line}: ${row.name} is out of order, and the manifest has ${order[index]} here`);
            }
        });
    }
    return found;
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    if (process.argv.length > 2) {
        console.error('usage: node scripts/librarytable.ts');
        return 2;
    }
    const libraries = manifestLibraries(readFileSync(path.join(ROOT, MANIFEST), 'utf8'));
    const rows = tableRows(readFileSync(path.join(ROOT, TABLE), 'utf8'));
    const found = problems(libraries, rows);
    for (const problem of found) {
        console.log(problem);
    }
    if (found.length > 0) {
        console.log(`::error::The table of libraries in ${TABLE} does not match ${MANIFEST}. Give each library one row, in the manifest's order.`);
        return 1;
    }
    return 0;
}

if (shouldRun(import.meta.url)) {
    process.exitCode = main();
}
