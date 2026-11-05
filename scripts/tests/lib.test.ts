/**
 * Tests for the modules in scripts/lib that every gate shares: the paths of the tree, running
 * git, and reading the command line.
 *
 * The git tests run in this repository, so they need its history, as every gate does. Run them
 * with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { existsSync } from 'node:fs';
import { join } from 'node:path';
import process from 'node:process';
import { test } from 'node:test';

import { type Finding, UsageError, formatFinding, options, report } from '../lib/cli.ts';
import { GateError, commit, git, gitOutcome, mergeBase, tracked, untracked } from '../lib/git.ts';
import { ROOT, isGenerated, isTestPath, isThirdParty } from '../lib/tree.ts';

test('tree finds the repository root', () => {
    assert.ok(existsSync(join(ROOT, 'package.json')));
    assert.ok(existsSync(join(ROOT, 'CMakeLists.txt')));
});

test('tree names third-party code by its directory, and only at the root', () => {
    for (const file of ['vendor/x/src/noise.h', 'ports/glm/portfile.cmake']) {
        assert.equal(isThirdParty(file), true, file);
    }
    for (const file of ['api/vendor/x.h', 'demos/voxel/src/World.h', 'vendored.h']) {
        assert.equal(isThirdParty(file), false, file);
    }
});

test('tree names generated files at the root, and an install below it', () => {
    for (const file of ['out/build/x.h', 'node_modules/typescript/lib.d.ts', 'vcpkg_installed/x64/a.h', 'examples/starter/vcpkg_installed/a.h']) {
        assert.equal(isGenerated(file), true, file);
    }
    for (const file of ['api/out/x.h', 'scripts/output.ts']) {
        assert.equal(isGenerated(file), false, file);
    }
});

test('tree names a test path by a tests directory at any depth, and not by a name', () => {
    for (const file of ['tests/a.cpp', 'api/ui/tests/ScreenTest.cpp', 'scripts/tests/lib.test.ts']) {
        assert.equal(isTestPath(file), true, file);
    }
    for (const file of ['api/ui/Screen.cpp', 'api/latests/a.cpp', 'api/tests.cpp', 'tests']) {
        assert.equal(isTestPath(file), false, file);
    }
});

test('git returns what a command prints, and a failure throws a GateError with its first line', () => {
    assert.match(git(['rev-parse', '--show-toplevel']), /\S/);
    assert.throws(() => git(['rev-parse', '--verify', 'no-such-ref-at-all']), (error: unknown) =>
        error instanceof GateError && /^git rev-parse --verify no-such-ref-at-all failed: fatal/.test(error.message));
});

test('git reports a failed command without throwing, when asked how it ended', () => {
    const done = gitOutcome(['rev-parse', '--verify', '--quiet', 'no-such-ref-at-all']);
    assert.notEqual(done.code, 0);
    assert.equal(done.stdout, '');
});

test('git names a commit in full, and refuses what is not one', () => {
    assert.match(commit('HEAD'), /^[0-9a-f]{40}$/);
    assert.equal(commit('HEAD~1'), commit('HEAD^'));
    assert.throws(() => commit('no-such-ref-at-all'), { message: 'no-such-ref-at-all: not a commit' });
    // a tree is an object, and not a commit
    assert.throws(() => commit('HEAD^{tree}'), GateError);
});

test('git finds a merge base, and refuses a base that is not a commit', () => {
    assert.equal(mergeBase('HEAD'), commit('HEAD'));
    assert.equal(mergeBase('HEAD~1'), commit('HEAD~1'));
    assert.throws(() => mergeBase('no-such-ref-at-all'), { message: 'no-such-ref-at-all: not a commit' });
});

test('git lists tracked files by pathspec, relative to the root', () => {
    assert.deepEqual(tracked('package.json'), ['package.json']);
    assert.ok(tracked('*.cmd').includes('scripts/build.cmd'));
    assert.deepEqual(tracked('no-such-file'), []);
    assert.ok(untracked().every((file) => file !== '' && !file.startsWith('/')));
});

test('cli reads the options a gate takes, in either spelling', () => {
    const config = { base: { type: 'string' }, list: { type: 'boolean' } } as const;
    assert.deepEqual({ ...options('usage', config, false, ['--base', 'main', '--list']).values }, { base: 'main', list: true });
    assert.deepEqual({ ...options('usage', config, false, ['--base=main']).values }, { base: 'main' });
    assert.deepEqual({ ...options('usage', config, false, []).values }, {});
});

test('cli refuses an option the gate does not take, a missing or empty value, and a stray argument', () => {
    const config = { base: { type: 'string' } } as const;
    for (const args of [['--x'], ['--base'], ['--base='], ['--base', '  '], ['file.md'], ['-b', 'main']]) {
        assert.throws(() => options('usage: gate', config, false, args), (error: unknown) =>
            error instanceof UsageError && error.message === 'usage: gate', args.join(' '));
    }
    assert.throws(() => options('usage: gate', {}, false, ['--anything']), UsageError);
});

/** What report prints for some findings, and the exit status it returns. */
function reported(findings: Finding[]): { printed: string; status: number } {
    let printed = '';
    const write = process.stdout.write;
    process.stdout.write = ((chunk: string) => {
        printed += chunk;
        return true;
    }) as typeof process.stdout.write;
    try {
        const status = report(findings, 'breaks a rule in docs/Rules.md.');
        return { printed, status };
    } finally {
        process.stdout.write = write;
    }
}

test('cli prints a finding with its line, or without one when it has none', () => {
    assert.equal(formatFinding({ path: 'a/b.cpp', line: 12, rule: 'some-rule', message: 'what is wrong' }), 'a/b.cpp:12: some-rule: what is wrong');
    assert.equal(formatFinding({ path: 'a/CMakeLists.txt', line: null, rule: 'other', message: 'x' }), 'a/CMakeLists.txt: other: x');
});

test('cli reports nothing and exit 0 for no findings, and each finding and a summary otherwise', () => {
    assert.deepEqual(reported([]), { printed: '', status: 0 });
    const one = { path: 'a.md', line: 3, rule: 'r', message: 'm' };
    assert.deepEqual(reported([one]), {
        printed: 'a.md:3: r: m\n::error::1 finding above: each breaks a rule in docs/Rules.md.\n',
        status: 1,
    });
    assert.deepEqual(reported([one, { ...one, line: null }]), {
        printed: 'a.md:3: r: m\na.md: r: m\n::error::2 findings above: each breaks a rule in docs/Rules.md.\n',
        status: 1,
    });
});

test('cli takes positional arguments only from a gate that allows them', () => {
    const config = { all: { type: 'boolean' } } as const;
    assert.deepEqual(options('usage', config, true, ['--all', 'a.md', 'b.md']).positionals, ['a.md', 'b.md']);
    assert.throws(() => options('usage', config, false, ['--all', 'a.md']), UsageError);
});
