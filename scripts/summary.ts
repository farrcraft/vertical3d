/**
 * Check that docs/SUMMARY.md lists each page of the documentation site once.
 *
 * The site holds the pages SUMMARY.md lists, so a page it leaves out is not on the site. Every
 * Markdown file under docs/ is a page, except SUMMARY.md itself and the records under a
 * completed/ directory, which are frozen. The check reports:
 *
 *   unlisted      a page that SUMMARY.md does not list.
 *   no-page       an entry that names a file that is not a page.
 *   second-entry  a page that SUMMARY.md lists twice.
 *
 * Usage: node scripts/summary.ts
 */

import { readFileSync } from 'node:fs';
import * as path from 'node:path';

import { type Finding, options, report, runGate } from './lib/cli.ts';
import { tracked, untracked } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';

const SUMMARY = 'docs/SUMMARY.md';

/** Each page of the site, relative to docs/, from a list of files relative to the root. */
export function pages(files: readonly string[]): string[] {
    return files
        .filter((file) => file.startsWith('docs/') && file.endsWith('.md') && file !== SUMMARY)
        .filter((file) => !file.split('/').includes('completed'))
        .map((file) => file.slice('docs/'.length))
        .sort();
}

/** One entry of SUMMARY.md: the page it links, and its line. */
export interface Entry {
    page: string;
    line: number;
}

/** The entries of SUMMARY.md, in order. */
export function entries(summary: string): Entry[] {
    const found: Entry[] = [];
    summary.split(/\r?\n/).forEach((text, index) => {
        for (const match of text.matchAll(/\]\(([^)#\s]+)\)/g)) {
            found.push({ page: match[1] ?? '', line: index + 1 });
        }
    });
    return found;
}

/** What is wrong with SUMMARY.md, one finding a problem. */
export function problems(all: readonly string[], listed: readonly Entry[]): Finding[] {
    const found: Finding[] = [];
    const known = new Set(all);
    const seen = new Set<string>();
    for (const entry of listed) {
        if (!known.has(entry.page)) {
            found.push({ path: SUMMARY, line: entry.line, rule: 'no-page', message: `${entry.page} is not a page under docs/` });
        } else if (seen.has(entry.page)) {
            found.push({ path: SUMMARY, line: entry.line, rule: 'second-entry', message: `${entry.page} is listed already` });
        }
        seen.add(entry.page);
    }
    for (const page of all) {
        if (!seen.has(page)) {
            found.push({ path: `docs/${page}`, line: null, rule: 'unlisted', message: `${SUMMARY} does not list it` });
        }
    }
    return found;
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/summary.ts', {});
    const all = pages([...tracked('docs'), ...untracked()]);
    const listed = entries(readFileSync(path.join(ROOT, SUMMARY), 'utf8'));
    return report(problems(all, listed), `leaves ${SUMMARY} out of step with the pages under docs/. List each page once.`);
}

runGate(import.meta.url, main);
