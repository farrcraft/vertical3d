/**
 * Tests that start each review gate as a process, as CI starts it.
 *
 * A gate runs its command line only when Node is started with its file, so that a test can import
 * it. A gate that never ran would exit 0, and CI would pass it. Each gate is therefore started with
 * an input it must refuse, and has to exit with status 2. The gates are listed from the files in
 * scripts/, so a new gate is tested without being named here. Run them with
 * node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { mkdtempSync, readdirSync, rmSync, symlinkSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import process from 'node:process';
import { test } from 'node:test';

import { supportedNode } from '../lib/entry.ts';
import { ROOT } from '../lib/tree.ts';

/** Every gate: each TypeScript file directly in scripts/. */
const GATES = readdirSync(join(ROOT, 'scripts')).filter((name) => name.endsWith('.ts')).sort();

/** The gates that compare against a base revision named by --base. */
const BASED = ['boundary.ts', 'commentsonly.ts', 'failsfirst.ts', 'prose.ts'];

/** Starts a gate with the given arguments and Node options, from the repository root. */
function start(script: string, args: string[], nodeOptions: string[] = []) {
    return spawnSync(process.execPath, [...nodeOptions, script, ...args], { cwd: ROOT, encoding: 'utf8' });
}

test('every gate is listed', () => {
    for (const gate of ['linkrule.ts', 'prose.ts', 'failsfirst.ts', 'npm.ts', ...BASED]) {
        assert.ok(GATES.includes(gate), gate);
    }
});

for (const gate of GATES) {
    test(`${gate} runs when Node is started with it, and refuses an option it does not take`, () => {
        const run = start(join(ROOT, 'scripts', gate), ['--no-such-option']);
        assert.equal(run.status, 2, run.stderr);
        assert.match(run.stderr, new RegExp(`^usage: node scripts/${gate.replace('.', '\\.')}`));
        assert.equal(run.stdout, '');
    });
}

for (const gate of BASED) {
    test(`${gate} stops with exit 2 on a base that is not a commit, and names it`, () => {
        const run = start(join(ROOT, 'scripts', gate), ['--base', 'no-such-ref']);
        assert.equal(run.status, 2, run.stderr);
        assert.match(run.stderr, new RegExp(`^${gate.replace('.', '\\.')}: no-such-ref: not a commit`));
    });
}

// Node resolves a module's own path through a link, and does not resolve the path it was started
// with through links. A gate started through a junction or a symbolic link still has to see that
// it runs.
test('a gate started through a linked directory runs', () => {
    const scratch = mkdtempSync(join(tmpdir(), 'v3d-gates-'));
    const linked = join(scratch, 'scripts');
    try {
        symlinkSync(join(ROOT, 'scripts'), linked, process.platform === 'win32' ? 'junction' : 'dir');
        const run = start(join(linked, 'linkrule.ts'), ['--no-such-option']);
        assert.equal(run.status, 2, run.stderr);
    } finally {
        rmSync(scratch, { recursive: true, force: true });
    }
});

test('the gates support Node 24 and later', () => {
    assert.equal(supportedNode('24.0.0'), true);
    assert.equal(supportedNode('24.19.0'), true);
    assert.equal(supportedNode('25.1.0'), true);
    assert.equal(supportedNode('23.11.0'), false);
    assert.equal(supportedNode('22.18.0'), false);
    assert.equal(supportedNode('not a version'), false);
});

// The version is replaced before the gate loads, so an earlier Node is not needed to run this.
test('a gate started on an earlier Node stops with exit 2 and says so', () => {
    const older = "data:text/javascript,Object.defineProperty(process,'versions',{value:{...process.versions,node:'22.0.0'}})";
    const run = start(join(ROOT, 'scripts', 'linkrule.ts'), [], ['--import', older]);
    assert.equal(run.status, 2, run.stderr);
    assert.match(run.stderr, /Node 24 or later/);
});
