/**
 * Check that no directory of C++ code holds more headers than the limit in
 * docs/contributing/Conventions.md, under "Directories", unless COHESIVE lists it.
 *
 * The script counts the tracked headers directly in each directory, not in its subdirectories, but
 * those under a tests directory and the vendored ones. A header is counted rather than a source,
 * because each class has one header and may have no source of its own. It reports:
 *
 *   over-limit     a directory with more headers than LIMIT that COHESIVE does not list. Split it
 *                  by purpose, or list it with the family its headers belong to.
 *   stale-listing  a directory COHESIVE lists that is within the limit or holds no headers, so the
 *                  listing no longer says anything.
 *
 * Usage: node scripts/dirsize.ts
 */

import { options, runGate } from './lib/cli.ts';
import { tracked } from './lib/git.ts';
import { isTestPath, isThirdParty } from './lib/tree.ts';

/** The most headers a directory may hold before it has to be split or listed. */
export const LIMIT = 15;

/**
 * The directories allowed more than LIMIT headers, each with the family every header in it
 * belongs to. A directory belongs here only when no split by purpose exists, because each header
 * is one member of a family that is used together.
 */
export const COHESIVE: ReadonlyMap<string, string> = new Map([
    ['api/render/offline/sl/syntax', 'the shading language syntax tree, one header per kind of node'],
    ['api/ui/component', 'the widgets, one header per widget'],
    ['vertical3d/src/command', 'the editor commands, one header per command family'],
]);

export type Rule = 'over-limit' | 'stale-listing';

/** One report, and the directory it is about. */
export interface Finding {
    directory: string;
    rule: Rule;
    message: string;
}

/** Whether a path is counted: a header that is neither third-party code nor a test. */
export function counted(file: string): boolean {
    return file.endsWith('.h') && !isTestPath(file) && !isThirdParty(file);
}

/** How many counted headers are directly in each directory. A file at the root is in ".". */
export function headerCounts(files: readonly string[]): Map<string, number> {
    const counts = new Map<string, number>();
    for (const file of files) {
        if (!counted(file)) {
            continue;
        }
        const at = file.lastIndexOf('/');
        const directory = at < 0 ? '.' : file.slice(0, at);
        counts.set(directory, (counts.get(directory) ?? 0) + 1);
    }
    return counts;
}

/** The findings for a set of header counts, against a limit and a list of cohesive directories. */
export function check(counts: ReadonlyMap<string, number>, limit: number, cohesive: ReadonlyMap<string, string>): Finding[] {
    const findings: Finding[] = [];
    for (const [directory, count] of [...counts].sort(([a], [b]) => a.localeCompare(b))) {
        if (count > limit && !cohesive.has(directory)) {
            findings.push({ directory, rule: 'over-limit', message: `${count} headers, over the limit of ${limit}` });
        }
    }
    for (const directory of [...cohesive.keys()].sort()) {
        const count = counts.get(directory) ?? 0;
        if (count <= limit) {
            findings.push({ directory, rule: 'stale-listing', message: `listed as cohesive, but holds ${count} headers, within the limit of ${limit}` });
        }
    }
    return findings;
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/dirsize.ts', {});
    const findings = check(headerCounts(tracked('*.h')), LIMIT, COHESIVE);
    for (const finding of findings) {
        console.log(`${finding.directory}: ${finding.rule}: ${finding.message}`);
    }
    if (findings.length > 0) {
        console.log(`::error::${findings.length} directories above break "Directories" in docs/contributing/Conventions.md. Split a directory by purpose, or list it in COHESIVE in scripts/dirsize.ts with the family its headers belong to.`);
        return 1;
    }
    return 0;
}

runGate(import.meta.url, main);
