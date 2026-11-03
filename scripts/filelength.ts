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

import { readFileSync } from 'node:fs';
import * as path from 'node:path';

import { options, runGate } from './lib/cli.ts';
import { tracked } from './lib/git.ts';
import { ROOT, isTestPath, isThirdParty } from './lib/tree.ts';

/** The most lines a file may have. */
export const LIMIT = 1000;

/** Whether a path is checked: a C++ file that is neither third-party code nor a test. */
export function checked(file: string): boolean {
    return /\.(h|cpp|cxx)$/.test(file) && !isTestPath(file) && !isThirdParty(file);
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
    options('usage: node scripts/filelength.ts', {});
    const files = tracked('*.h', '*.cpp', '*.cxx').filter(checked);
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

runGate(import.meta.url, main);
