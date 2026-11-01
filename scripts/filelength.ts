/**
 * Check that no C++ file but a test is longer than the limit in docs/contributing/Conventions.md,
 * under "Size".
 *
 * The script reads every tracked .h, .cpp and .cxx file but the vendored ones and those under a
 * tests directory, and reports each that has more lines than LIMIT. A line is a line of the file,
 * whatever it holds, so comments and blank lines count.
 *
 * Usage: node scripts/filelength.ts
 */

import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { shouldRun } from './entry.ts';

const ROOT = path.resolve(import.meta.dirname, '..');

/** The most lines a file may have. */
export const LIMIT = 1000;

/** Directories whose files are not this tree's code. */
const VENDORED = ['vendor/', 'demos/voxel/src/noise/'];

/** Whether a path is checked: a C++ file that is neither vendored nor a test. */
export function checked(file: string): boolean {
    return /\.(h|cpp|cxx)$/.test(file) && !/(^|\/)tests\//.test(file) && !VENDORED.some((v) => file.startsWith(v));
}

/** How many lines a text has. A final line break ends the last line rather than starting one. */
export function lineCount(text: string): number {
    if (text === '') {
        return 0;
    }
    const breaks = text.split('\n').length - 1;
    return text.endsWith('\n') ? breaks : breaks + 1;
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    if (process.argv.length > 2) {
        console.error('usage: node scripts/filelength.ts');
        return 2;
    }
    const files = execFileSync('git', ['ls-files', '-z', '--', '*.h', '*.cpp', '*.cxx'], { cwd: ROOT, encoding: 'utf-8', maxBuffer: 64 * 1024 * 1024 })
        .split('\0')
        .filter((f) => f !== '' && checked(f));
    let count = 0;
    for (const file of files) {
        const lines = lineCount(readFileSync(path.join(ROOT, file), 'utf8'));
        if (lines > LIMIT) {
            console.log(`${file}: ${lines} lines, over the limit of ${LIMIT}`);
            count++;
        }
    }
    if (count > 0) {
        console.log(`::error::${count} files above are longer than docs/contributing/Conventions.md allows. Split each into classes with one job each.`);
        return 1;
    }
    return 0;
}

if (shouldRun(import.meta.url)) {
    process.exitCode = main();
}
