/**
 * Printing what the fails-first harness found.
 */

import process from 'node:process';

import type { Case } from './types.ts';

export function printTable(cases: Case[], withResult: boolean): void {
    const headers = ['case', 'suite', 'file', ...(withResult ? ['result'] : [])];
    const rows = cases.map((c) => {
        const row = [c.path, c.suite, c.file];
        if (withResult) {
            row.push((c.result ?? '') + (c.note ? ` (${c.note})` : ''));
        }
        return row;
    });
    const widths = headers.map((h, i) => Math.max(h.length, ...rows.map((r) => r[i].length)));
    const line = (cells: string[]): string => cells.map((cell, i) => cell.padEnd(widths[i])).join('  ');
    const out = [line(headers), line(widths.map((w) => '-'.repeat(w))), ...rows.map(line)];
    process.stdout.write(out.join('\n') + '\n');
}

// The reason each case that states one gives for passing without the change, and what each case
// that covers the base says it covers.
export function printStated(cases: Case[]): void {
    const stated = cases.filter((c) => c.stated !== null);
    if (stated.length > 0) {
        console.log();
        console.log('Stated to pass before the change:');
        for (const c of stated) {
            console.log(`  ${c.suite} ${c.path}: ${c.stated}`);
        }
    }
    const covering = cases.filter((c) => c.stated === null && c.claim !== null);
    if (covering.length > 0) {
        console.log();
        console.log('Stated to cover code already in the base:');
        for (const c of covering) {
            const commit = c.covers === null ? 'names no commit the base contains' : c.covers.slice(0, 9);
            console.log(`  ${c.suite} ${c.path} (${commit}): ${c.claim}`);
        }
    }
}
