/**
 * The revisions the fails-first harness compares, read through git.
 */

import { GateError, gitOutcome } from '../lib/git.ts';
import { ROOT } from '../lib/tree.ts';
import type { Change } from './types.ts';

// Runs git with every path taken literally, so that a file name with a glob character in it
// names only that file.
export function git(args: string[], cwd: string = ROOT, check: boolean = true): { code: number; stdout: string } {
    const done = gitOutcome(['--literal-pathspecs', ...args], cwd);
    if (check && done.code !== 0) {
        throw new GateError(`git ${args.join(' ')} failed:\n${done.stderr.trim()}`);
    }
    return done;
}

// The file's text at the revision, or null when it does not exist there.
export function show(revision: string, file: string): string | null {
    const done = git(['show', `${revision}:${file}`], ROOT, false);
    return done.code === 0 ? done.stdout : null;
}

// Every file the changeset touched. A rename is a delete and an add.
export function changedFiles(base: string, head: string): Change[] {
    const fields = git(['diff', '--name-status', '--no-renames', '-z', base, head]).stdout
        .split('\0').filter((f) => f.length > 0);
    const changes: Change[] = [];
    for (let i = 0; i + 1 < fields.length; i += 2) {
        changes.push({ status: fields[i][0], file: fields[i + 1] });
    }
    return changes;
}
