/**
 * The worktree the change is built in: the head revision with everything outside tests/ at
 * the base.
 */

import * as fs from 'node:fs';

import { ROOT, isTestPath } from '../lib/tree.ts';
import { dropSource, listedSources } from './cmake.ts';
import { git } from './repo.ts';
import type { Change, Drop } from './types.ts';

// Makes the worktree: the head revision with every file outside tests/ at its base version.
// Returns, for each suite, the sources dropped from its list because the change added them.
export function makeWorktree(head: string, base: string, changes: Change[], worktree: string): Map<string, Drop[]> {
    console.log(`worktree: ${worktree}`);
    git(['worktree', 'add', '--detach', worktree, head]);
    // a deleted file comes back even under tests/, because a base CMakeLists.txt may add the
    // test directory it was in. A head list does not name it, so it builds into nothing
    const restore = changes.filter((c) => c.status === 'D' || (!isTestPath(c.file) && c.status !== 'A')).map((c) => c.file);
    const remove = changes.filter((c) => !isTestPath(c.file) && c.status === 'A').map((c) => c.file);
    for (let i = 0; i < restore.length; i += 100) {
        git(['checkout', base, '--', ...restore.slice(i, i + 100)], worktree);
    }
    for (let i = 0; i < remove.length; i += 100) {
        git(['rm', '-q', '--', ...remove.slice(i, i + 100)], worktree);
    }
    console.log(`restored ${restore.length} files to ${base.slice(0, 9)}, removed ${remove.length} non-test files it added`);

    // a head list that names a source the change added would stop the configure
    const removed = new Set(remove);
    const dropped = new Map<string, Drop[]>();
    for (const { suite, directory, source } of listedSources(head)) {
        if (removed.has(source) && dropSource(worktree, directory, suite, source)) {
            console.log(`dropped ${source} from v3dtest_${suite}: the change added it`);
            dropped.set(suite, [...(dropped.get(suite) ?? []), { source, added: true }]);
        }
    }
    return dropped;
}

export function removeWorktree(worktree: string): void {
    if (git(['worktree', 'remove', '--force', worktree], ROOT, false).code !== 0) {
        fs.rmSync(worktree, { recursive: true, force: true });
        git(['worktree', 'prune'], ROOT, false);
    }
}
