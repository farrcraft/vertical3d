/**
 * Running git for the review gates, and finding the revision a change is measured from.
 *
 * Every command runs at the repository root unless a caller names another directory. A command
 * that fails stops the gate with a GateError, which the gate reports with exit status 2.
 */

import { spawnSync } from 'node:child_process';

import { ROOT } from './tree.ts';

/** A reason a gate cannot run. The gate prints it as one line and exits with status 2. */
export class GateError extends Error {}

/** Enough for a diff of the whole tree. */
const MAX_BUFFER = 1 << 30;

/** How a git command ended: its exit status and what it printed. */
export interface Outcome {
    code: number;
    stdout: string;
    stderr: string;
}

/** Runs git and returns how it ended, whatever its status. */
export function gitOutcome(args: readonly string[], cwd: string = ROOT): Outcome {
    const done = spawnSync('git', args, { cwd, encoding: 'utf8', maxBuffer: MAX_BUFFER });
    if (done.error !== undefined) {
        throw new GateError(`git ${args.join(' ')} could not run: ${done.error.message}`);
    }
    return { code: done.status ?? 1, stdout: done.stdout, stderr: done.stderr };
}

/** Runs git and returns what it printed. A failure throws a GateError with git's first line. */
export function git(args: readonly string[], cwd: string = ROOT): string {
    const done = gitOutcome(args, cwd);
    if (done.code !== 0) {
        const first = done.stderr.trim().split('\n')[0] ?? '';
        throw new GateError(`git ${args.join(' ')} failed${first ? ': ' + first : ''}`);
    }
    return done.stdout;
}

/** Whether a git command succeeds. */
export function succeeds(args: readonly string[], cwd: string = ROOT): boolean {
    return gitOutcome(args, cwd).code === 0;
}

/** The full name of the commit a revision names. Anything else throws a GateError. */
export function commit(revision: string): string {
    const done = gitOutcome(['rev-parse', '--verify', '--quiet', `${revision}^{commit}`]);
    if (done.code !== 0) {
        throw new GateError(`${revision}: not a commit`);
    }
    return done.stdout.trim();
}

/**
 * The commit where HEAD left a branch, so that commits the branch gained later are not counted
 * as the change's. With no branch named, that is origin/main, or main when there is no
 * origin/main.
 */
export function mergeBase(branch?: string): string {
    if (branch !== undefined) {
        commit(branch);
        const done = gitOutcome(['merge-base', 'HEAD', branch]);
        if (done.code !== 0) {
            throw new GateError(`${branch}: no merge base with HEAD`);
        }
        return done.stdout.trim();
    }
    for (const candidate of ['origin/main', 'main']) {
        const done = gitOutcome(['merge-base', 'HEAD', candidate]);
        if (done.code === 0) {
            return done.stdout.trim();
        }
    }
    throw new GateError('no merge base of HEAD with origin/main or main; pass --base');
}

/** The tracked files that match the pathspecs, relative to the root. */
export function tracked(...pathspecs: string[]): string[] {
    return git(['ls-files', '-z', '--', ...pathspecs]).split('\0').filter((file) => file !== '');
}

/** The files neither tracked nor ignored, relative to the root. A change can add them. */
export function untracked(): string[] {
    return git(['ls-files', '-z', '--others', '--exclude-standard']).split('\0').filter((file) => file !== '');
}
