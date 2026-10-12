/**
 * Check that every action a workflow uses is pinned to a full commit sha.
 *
 * The script reads every workflow in .github/workflows and every action defined under
 * .github/actions, with either the .yml or the .yaml extension. Each uses: key is read whether it
 * is written as a block mapping or inside a flow mapping such as `- {uses: actions/checkout@...}`,
 * and whether its value is plain, single quoted or double quoted. A comment is not read.
 *
 * A value is pinned when it names an action and a 40 character lowercase hex sha after the @. A
 * local action, a path starting ./ in this repository, needs no sha. Anything else is reported:
 * a tag, a branch, a short sha, an uppercase sha, a docker:// image, and a uses: with no value
 * on its line.
 *
 * Usage: node scripts/pinned.ts
 */

import { existsSync, readdirSync, readFileSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { shouldRun } from './entry.ts';

const ROOT = path.resolve(import.meta.dirname, '..');

/** A uses: value that is not pinned, and the line it is on. */
export interface Unpinned {
    line: number;
    value: string;
}

/** The text of a YAML line before its comment, if it has one outside a quoted string. */
function beforeComment(line: string): string {
    let quote: string | null = null;
    for (let i = 0; i < line.length; i++) {
        const c = line[i];
        if (quote !== null) {
            if (c === quote) {
                quote = null;
            }
        } else if (c === '"' || c === "'") {
            quote = c;
        } else if (c === '#' && (i === 0 || /\s/.test(line[i - 1]))) {
            return line.slice(0, i);
        }
    }
    return line;
}

/** Whether a uses: value names a local action or a full commit sha. */
export function isPinned(value: string): boolean {
    return value.startsWith('./') || /^[^@\s]+@[0-9a-f]{40}$/.test(value);
}

/** Every uses: value in a workflow or action file that is not pinned. */
export function unpinned(text: string): Unpinned[] {
    // the key starts a line, follows a sequence dash, or follows the { or , of a flow mapping
    const key = /(?:^\s*(?:-\s*)?|[{,]\s*)uses\s*:(?=\s|$)\s*(?:"([^"]*)"|'([^']*)'|([^\s,}]*))/g;
    const found: Unpinned[] = [];
    text.split(/\r?\n/).forEach((raw, index) => {
        const line = beforeComment(raw);
        for (const match of line.matchAll(key)) {
            const value = match[1] ?? match[2] ?? match[3] ?? '';
            if (!isPinned(value)) {
                found.push({ line: index + 1, value });
            }
        }
    });
    return found;
}

/** The workflow and action files under a repository root, relative to it. */
export function files(root: string): string[] {
    const yaml = (name: string) => name.endsWith('.yml') || name.endsWith('.yaml');
    const found: string[] = [];
    const workflows = path.join(root, '.github', 'workflows');
    if (existsSync(workflows)) {
        for (const entry of readdirSync(workflows, { withFileTypes: true })) {
            if (entry.isFile() && yaml(entry.name)) {
                found.push(path.join(workflows, entry.name));
            }
        }
    }
    const actions = path.join(root, '.github', 'actions');
    if (existsSync(actions)) {
        for (const entry of readdirSync(actions, { withFileTypes: true, recursive: true })) {
            if (entry.isFile() && (entry.name === 'action.yml' || entry.name === 'action.yaml')) {
                found.push(path.join(entry.parentPath, entry.name));
            }
        }
    }
    return found.map((f) => path.relative(root, f).split(path.sep).join('/')).sort();
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    if (process.argv.length > 2) {
        console.error('usage: node scripts/pinned.ts');
        return 2;
    }
    let count = 0;
    for (const file of files(ROOT)) {
        for (const { line, value } of unpinned(readFileSync(path.join(ROOT, file), 'utf8'))) {
            console.log(`${file}:${line}: uses: ${value === '' ? '(no value on the line)' : value}`);
            count++;
        }
    }
    if (count > 0) {
        console.log('::error::Each uses: above is not pinned. Pin it to the full commit sha its tag points at.');
        return 1;
    }
    return 0;
}

if (shouldRun(import.meta.url)) {
    process.exitCode = main();
}
