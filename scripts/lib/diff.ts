/**
 * The lines a change adds, read from git's diff of the working tree against a base.
 *
 * The working tree includes what is staged, what is not, and the files git does not track yet.
 * A file it does not track counts as new in its entirety. A file the change deletes adds nothing.
 */

import { Buffer } from 'node:buffer';

import { git, untracked } from './git.ts';
import { ROOT } from './tree.ts';

/**
 * The options that make git print a diff this module can read, whatever the user's
 * configuration. They fix the a/ and b/ prefixes, and turn off colour, external and
 * text-converting drivers, and rename detection.
 */
export const DIFF_OPTIONS: readonly string[] = ['--src-prefix=a/', '--dst-prefix=b/', '--no-color', '--no-ext-diff',
    '--no-textconv', '--no-renames'];

/** The escapes git's C-style quoting of a path uses, other than an octal byte. */
const ESCAPES: Record<string, number> = { a: 7, b: 8, t: 9, n: 10, v: 11, f: 12, r: 13, '"': 34, '\\': 92 };

/** The lines a path adds: their numbers in the working tree, or null when the whole file is new. */
export type Added = Set<number> | null;

/** What a change adds: the lines of each path, and the paths that are new files. */
export interface Change {
    added: Map<string, Added>;
    newFiles: Set<string>;
}

/**
 * The path a diff header line such as "+++ b/name" names, or null for /dev/null. Git quotes a
 * name that holds a control character, a quote, a backslash or a byte above 127, and ends an
 * unquoted name that holds a space with a tab.
 */
export function headerPath(header: string): string | null {
    let name = header.slice(4);
    const quoted = /^"((?:[^"\\]|\\.)*)"/.exec(name);
    if (quoted) {
        const bytes: number[] = [];
        const body = quoted[1] ?? '';
        for (let index = 0; index < body.length; index++) {
            const char = body[index] ?? '';
            const octal = body.slice(index + 1, index + 4);
            if (char !== '\\') {
                bytes.push(...Buffer.from(char, 'utf8'));
            } else if (/^[0-7]{3}$/.test(octal)) {
                bytes.push(parseInt(octal, 8));
                index += 3;
            } else {
                index += 1;
                bytes.push(ESCAPES[body[index] ?? ''] ?? body.charCodeAt(index));
            }
        }
        name = Buffer.from(bytes).toString('utf8');
    } else {
        name = name.replace(/\t$/, '');
    }
    return name.startsWith('b/') ? name.slice(2) : null;
}

/**
 * The lines each path adds in a diff printed with --unified=0 and DIFF_OPTIONS. A path the diff
 * deletes is not in the result, and one whose hunks only remove lines maps to an empty set.
 */
export function parseAdded(diff: string): Map<string, Set<number>> {
    const added = new Map<string, Set<number>>();
    let current: Set<number> | null = null;
    // the added lines left in the current hunk, from its header. With no context, a hunk lists
    // its removed lines and then its added ones, so the hunk ends with its last added line. An
    // added line is never a file header, even when it starts "++ ", and a removed line starts
    // with "-", so it cannot be read as one
    let newLeft = 0;
    for (const raw of diff.split('\n')) {
        const line = raw.replace(/\r$/, '');
        if (newLeft > 0 && line.startsWith('+')) {
            newLeft -= 1;
            continue;
        }
        if (line.startsWith('+++ ')) {
            const target = headerPath(line);
            current = null;
            if (target !== null) {
                current = added.get(target) ?? new Set<number>();
                added.set(target, current);
            }
        } else if (line.startsWith('@@')) {
            const match = /^@@ -\d+(?:,\d+)? \+(\d+)(?:,(\d+))? @@/.exec(line);
            if (match) {
                const start = Number(match[1]);
                newLeft = match[2] === undefined ? 1 : Number(match[2]);
                for (let number = start; number < start + newLeft && current !== null; number++) {
                    current.add(number);
                }
            }
        }
    }
    return added;
}

/** The paths a git diff --name-status -z output names as added. */
export function addedFiles(nameStatus: string): Set<string> {
    const found = new Set<string>();
    const fields = nameStatus.split('\0');
    for (let index = 0; index + 1 < fields.length; index += 2) {
        if (fields[index] === 'A') {
            found.add(fields[index + 1] ?? '');
        }
    }
    found.delete('');
    return found;
}

/** What the working tree in a repository adds since the base commit. */
export function changeSince(base: string, cwd: string = ROOT): Change {
    const newFiles = addedFiles(git(['diff', '--name-status', '-z', ...DIFF_OPTIONS, base], cwd));
    const added = new Map<string, Added>(parseAdded(git(['diff', '--unified=0', ...DIFF_OPTIONS, base], cwd)));
    for (const path of untracked(cwd)) {
        newFiles.add(path);
        added.set(path, null);
    }
    return { added, newFiles };
}
