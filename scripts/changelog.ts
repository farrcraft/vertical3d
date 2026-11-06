/**
 * Check that a change to what a release ships adds its entry to CHANGELOG.md.
 *
 * A release's notes are CHANGELOG.md's section for it, so a change a user would notice has to say
 * so there. A release ships the api's sources, the apps, the demos and the shared data, but not
 * their tests or their READMEs. The check reports:
 *
 *   no-entry       the change touches something a release ships and adds no line to the
 *                  changelog.
 *   no-unreleased  the changelog has no Unreleased section for the entry to go in.
 *
 * Usage: node scripts/changelog.ts [--base REF]
 *
 * The changeset is every commit since the merge base with origin/main, or with main when there is
 * no origin/main, plus the working tree and untracked files. --base names another branch or
 * commit, and the changeset starts at its merge base with HEAD.
 */

import { readFileSync } from 'node:fs';
import * as path from 'node:path';

import { type Finding, options, report, runGate } from './lib/cli.ts';
import { git, mergeBase, untracked } from './lib/git.ts';
import { ROOT, isTestPath } from './lib/tree.ts';

const CHANGELOG = 'CHANGELOG.md';

/** The directories whose files a release ships, below their tests and READMEs. */
const SHIPPED = ['api/', 'vertical3d/', 'moya/', 'imagetool/', 'demos/', 'data/'];

/** Whether a release ships the file: one under a shipped directory, outside tests, not Markdown. */
export function ships(file: string): boolean {
    return SHIPPED.some((directory) => file.startsWith(directory)) && !isTestPath(file) && !file.endsWith('.md');
}

/** Whether the changelog has the section a new entry goes in. */
export function hasUnreleased(changelog: string): boolean {
    return /^## \[Unreleased\]\s*$/m.test(changelog);
}

/** What is wrong with a change: the files it changed, and the changelog as it stands. */
export function problems(changed: readonly string[], changelog: string): Finding[] {
    const found: Finding[] = [];
    if (!hasUnreleased(changelog)) {
        found.push({ path: CHANGELOG, line: null, rule: 'no-unreleased', message: 'there is no "## [Unreleased]" section' });
    }
    const shipped = changed.filter(ships);
    if (shipped.length > 0 && !changed.includes(CHANGELOG)) {
        const named = shipped.length === 1 ? shipped[0] : `${shipped[0]} and ${shipped.length - 1} more`;
        found.push({ path: CHANGELOG, line: null, rule: 'no-entry', message: `the change touches ${named}, and adds no entry` });
    }
    return found;
}

function main(): number {
    const { values } = options('usage: node scripts/changelog.ts [--base REF]', { base: { type: 'string' } });
    const base = mergeBase(values.base);
    const changed = [...git(['diff', '--name-only', '-z', base]).split('\0'), ...untracked()].filter((file) => file !== '');
    const changelog = readFileSync(path.join(ROOT, CHANGELOG), 'utf8');
    return report(problems(changed, changelog), 'leaves a change a release ships out of CHANGELOG.md. Add its entry under Unreleased.');
}

runGate(import.meta.url, main);
