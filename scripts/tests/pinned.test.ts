/**
 * Tests for scripts/pinned.ts, the check that every action a workflow uses is pinned to a commit.
 *
 * Each test feeds the check one way of writing a uses: key. Run them with
 * node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import process from 'node:process';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { files, unpinned } from '../pinned.ts';

const SHA = '11d5960a326750d5838078e36cf38b85af677262';

/** The values the check reports for a file, as "line: value" strings. */
function reported(lines: string[]): string[] {
    return unpinned(lines.join('\n')).map((u) => `${u.line}: ${u.value}`);
}

test('pinned accepts a full sha and a local action, and reports a tag', () => {
    const lines = [
        'steps:',
        `  - uses: actions/checkout@${SHA} # v4.4.0`,
        '  - uses: ./.github/actions/build',
        '  - uses: actions/checkout@v4',
    ];
    assert.deepEqual(reported(lines), ['4: actions/checkout@v4']);
});

test('pinned reports a short sha, an uppercase sha and a docker image', () => {
    const lines = [
        '  - uses: actions/checkout@11d5960',
        `  - uses: actions/checkout@${SHA.toUpperCase()}`,
        '  - uses: docker://alpine:3.20',
    ];
    assert.deepEqual(reported(lines), [
        '1: actions/checkout@11d5960',
        `2: actions/checkout@${SHA.toUpperCase()}`,
        '3: docker://alpine:3.20',
    ]);
});

test('pinned reads a uses: inside a flow mapping', () => {
    const lines = [
        '  - {uses: actions/checkout@v4}',
        '  - {name: Checkout, uses: actions/setup-node@v4}',
        `  - {uses: actions/checkout@${SHA}}`,
    ];
    assert.deepEqual(reported(lines), ['1: actions/checkout@v4', '2: actions/setup-node@v4']);
});

test('pinned reads a quoted value as the value inside the quotes', () => {
    const lines = [
        `  - uses: "actions/checkout@${SHA}"`,
        `  - uses: 'actions/checkout@${SHA}'`,
        '  - uses: "actions/checkout@v4"',
    ];
    assert.deepEqual(reported(lines), ['3: actions/checkout@v4']);
});

test('pinned reports a uses: with no value on its line', () => {
    assert.deepEqual(reported(['  - uses:', `    actions/checkout@${SHA}`]), ['1: ']);
});

test('pinned does not read a comment or a key that only ends in uses', () => {
    const lines = [
        '  # uses: actions/checkout@v4',
        `  - uses: actions/checkout@${SHA} # was uses: actions/checkout@v4`,
        '    reuses: something@v1',
    ];
    assert.deepEqual(reported(lines), []);
});

test('pinned reads .yml and .yaml workflows and the actions defined in the repository', () => {
    const root = mkdtempSync(join(tmpdir(), 'v3d-pinned-'));
    try {
        mkdirSync(join(root, '.github', 'workflows'), { recursive: true });
        mkdirSync(join(root, '.github', 'actions', 'build'), { recursive: true });
        for (const name of ['.github/workflows/ci.yml', '.github/workflows/release.yaml',
            '.github/workflows/notes.md', '.github/actions/build/action.yaml']) {
            writeFileSync(join(root, name), '');
        }
        assert.deepEqual(files(root), [
            '.github/actions/build/action.yaml',
            '.github/workflows/ci.yml',
            '.github/workflows/release.yaml',
        ]);
    } finally {
        rmSync(root, { recursive: true, force: true });
    }
});

test('pinned.ts runs when Node is started with it', () => {
    const root = fileURLToPath(new URL('../../', import.meta.url));
    const run = spawnSync(process.execPath, [join(root, 'scripts', 'pinned.ts'), '--no-such-option'],
        { cwd: root, encoding: 'utf8' });
    assert.equal(run.status, 2, run.stderr);
    assert.match(run.stderr, /usage/);
});
