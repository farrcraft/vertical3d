/**
 * Tests for the entry points of the review gates that read C++ through scripts/lib/lexer.ts.
 *
 * Most tests feed a gate a shape of C++ source that a lexer can misread. The last ones start each
 * gate as a process, to see that it runs. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { mkdtempSync, rmSync, symlinkSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import process from 'node:process';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { attachCode, check } from '../boundary.ts';
import { supportedNode } from '../lib/entry.ts';

/** The boundary reports for the given lines of one C++ file, as "line: rule" strings. */
function boundary(text: string, numbers: number[]): string[] {
    const path = 'api/example/Example.cxx';
    const lines = text.split('\n');
    const added = numbers.map((number) => ({ path, number, text: lines[number - 1] }));
    return check(attachCode(added, () => text)).map((r) => `${r.number}: ${r.rule}`);
}

test('boundary reports a cast on a line that starts with a dereference', () => {
    const text = 'void f(int* out, float x) {\n    *out = static_cast<int>(std::floor(x));\n}';
    assert.deepEqual(boundary(text, [2]), ['2: float-cast']);
});

test('boundary skips a cast inside a block comment that an unchanged line opened', () => {
    const text = '/* the old way:\n*out = static_cast<int>(std::floor(x));\n*/\n';
    assert.deepEqual(boundary(text, [2]), []);
});

test('boundary reports a cast on the line after a continued string', () => {
    const text = 'const char* s = "a \\\r\nb /* c";\r\nint n = static_cast<int>(std::floor(x));\r\n/* d */\r\n';
    assert.deepEqual(boundary(text, [3]), ['3: float-cast']);
});

// Each gate runs its command line when Node is started with it, and not when a test imports it.
// A gate that did not run would exit 0, so each is given an input it must refuse: a base that is
// not a commit, or an option it does not take. Each runs from the repository root, as CI runs it.
const ROOT = fileURLToPath(new URL('../../', import.meta.url));

/** Starts a gate with the given arguments, from the repository root. */
function start(script: string, args: string[]) {
    return spawnSync(process.execPath, [script, ...args], { cwd: ROOT, encoding: 'utf8' });
}

for (const gate of ['prose.ts', 'boundary.ts']) {
    test(`${gate} runs when Node is started with it`, () => {
        const run = start(join(ROOT, 'scripts', gate), ['--base', 'no-such-ref']);
        assert.equal(run.status, 2, run.stderr);
        assert.match(run.stderr, /no-such-ref/);
    });
}

test('failsfirst.ts runs when Node is started with it', () => {
    const run = start(join(ROOT, 'scripts', 'failsfirst.ts'), ['--no-such-option']);
    assert.equal(run.status, 2, run.stderr);
});

test('linkrule.ts runs when Node is started with it', () => {
    const run = start(join(ROOT, 'scripts', 'linkrule.ts'), ['--no-such-option']);
    assert.equal(run.status, 2, run.stderr);
    assert.match(run.stderr, /usage/);
});

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
    const run = spawnSync(process.execPath, ['--import', older, join(ROOT, 'scripts', 'linkrule.ts')], { cwd: ROOT, encoding: 'utf8' });
    assert.equal(run.status, 2, run.stderr);
    assert.match(run.stderr, /Node 24 or later/);
});
