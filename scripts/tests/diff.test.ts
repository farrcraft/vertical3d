/**
 * Tests for scripts/lib/diff.ts, which reads the lines a change adds from git's diff.
 *
 * Most tests feed the parser a diff written out by hand, each with a shape a simpler reading gets
 * wrong. The last ones make a repository in a temporary directory, so that git itself prints the
 * diff, quotes the paths and lists the untracked files. Run them with
 * node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { mkdtempSync, rmSync, unlinkSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { test } from 'node:test';

import { addedFiles, changeSince, headerPath, parseAdded } from '../lib/diff.ts';

/** The lines a diff adds, as "path: n, n" strings in the order the diff names the paths. */
function added(lines: string[]): string[] {
    return [...parseAdded(lines.join('\n'))].map(([path, numbers]) => `${path}: ${[...numbers].join(', ')}`);
}

test('diff reads a plain path, and one git ends with a tab because it holds a space', () => {
    assert.equal(headerPath('+++ b/api/ui/Screen.cpp'), 'api/ui/Screen.cpp');
    assert.equal(headerPath('+++ b/docs/a name.md\t'), 'docs/a name.md');
    assert.equal(headerPath('+++ /dev/null'), null);
});

test('diff decodes a quoted path: octal bytes of UTF-8, and the C escapes', () => {
    assert.equal(headerPath('+++ "b/docs/na\\303\\257ve.md"'), 'docs/naïve.md');
    assert.equal(headerPath('+++ "b/a\\"quote\\\\slash\\ttab.md"'), 'a"quote\\slash\ttab.md');
    // an unquoted character inside the quotes is kept, and a digit after an escape is not octal
    assert.equal(headerPath('+++ "b/x\\t1y.md"'), 'x\t1y.md');
    // with core.quotePath off, git leaves a character above 127 raw inside the quotes
    assert.equal(headerPath('+++ "b/naïve\\"q.md"'), 'naïve"q.md');
    // an escape with fewer than three octal digits is the character it escapes
    assert.equal(headerPath('+++ "b/x\\7.md"'), 'x7.md');
});

test('diff numbers the added lines of each hunk, across several hunks and files', () => {
    assert.deepEqual(added([
        'diff --git a/one.md b/one.md',
        '--- a/one.md',
        '+++ b/one.md',
        '@@ -2,0 +3,2 @@',
        '+first',
        '+second',
        '@@ -10 +12 @@',
        '-old',
        '+new',
        'diff --git a/two.md b/two.md',
        '--- a/two.md',
        '+++ b/two.md',
        '@@ -1 +1 @@',
        '-a',
        '+b',
    ]), ['one.md: 3, 4, 12', 'two.md: 1']);
});

test('diff reads an added line that starts "++ " as a line, and not as a header', () => {
    assert.deepEqual(added([
        '--- a/c.md',
        '+++ b/c.md',
        '@@ -0,0 +1,2 @@',
        '+++ b/other.md',
        '+--- a/other.md',
    ]), ['c.md: 1, 2']);
});

test('diff skips the marker of a missing newline, and reads the hunk after it', () => {
    assert.deepEqual(added([
        '--- a/d.md',
        '+++ b/d.md',
        '@@ -3 +3,2 @@',
        '-last',
        '\\ No newline at end of file',
        '+last',
        '++++ b/not-a-header.md',
        '\\ No newline at end of file',
        '@@ -9,0 +10 @@',
        '+later',
    ]), ['d.md: 3, 4, 10']);
});

test('diff gives a file that only loses lines no added lines, and a deleted file no entry', () => {
    assert.deepEqual(added([
        '--- a/e.md',
        '+++ b/e.md',
        '@@ -4,2 +3,0 @@',
        '-gone',
        '-gone too',
        '--- a/f.md',
        '+++ /dev/null',
        '@@ -1,2 +0,0 @@',
        '-x',
        '-y',
    ]), ['e.md: ']);
});

test('diff reads a header line that ends with a carriage return', () => {
    assert.deepEqual(added(['--- a/g.md\r', '+++ b/g.md\r', '@@ -0,0 +1 @@\r', '+line\r']), ['g.md: 1']);
});

test('diff lists the files a name-status output marks as added', () => {
    assert.deepEqual([...addedFiles('A\0new.md\0M\0changed.md\0D\0gone.md\0A\0a b.md\0')], ['new.md', 'a b.md']);
    assert.deepEqual([...addedFiles('')], []);
});

/** Runs git in a repository, and fails the test when it fails. */
function run(cwd: string, ...args: string[]): void {
    const done = spawnSync('git', args, { cwd, encoding: 'utf8' });
    assert.equal(done.status, 0, done.stderr);
}

/** A repository with one commit, which the callback changes before it is read. */
function withRepository(body: (directory: string, base: string) => void): void {
    const directory = mkdtempSync(join(tmpdir(), 'v3d-diff-'));
    try {
        run(directory, 'init', '-q');
        run(directory, 'config', 'user.email', 'gate@example.com');
        run(directory, 'config', 'user.name', 'gate');
        run(directory, 'config', 'core.autocrlf', 'false');
        writeFileSync(join(directory, 'kept.md'), 'one\ntwo\nthree\n');
        writeFileSync(join(directory, 'gone.md'), 'x\n');
        writeFileSync(join(directory, 'naïve name.md'), 'a\n');
        run(directory, 'add', '.');
        run(directory, 'commit', '-q', '-m', 'base');
        const base = spawnSync('git', ['rev-parse', 'HEAD'], { cwd: directory, encoding: 'utf8' }).stdout.trim();
        body(directory, base);
    } finally {
        rmSync(directory, { recursive: true, force: true });
    }
}

test('diff reads what a working tree adds: staged, unstaged, new, untracked and deleted', () => {
    withRepository((directory, base) => {
        writeFileSync(join(directory, 'kept.md'), 'one\ninserted\ntwo\nthree\nappended\n');
        writeFileSync(join(directory, 'naïve name.md'), 'a\nb\n');
        writeFileSync(join(directory, 'staged.md'), 's1\ns2\n');
        run(directory, 'add', 'staged.md');
        writeFileSync(join(directory, 'loose.md'), 'u\n');
        unlinkSync(join(directory, 'gone.md'));
        const change = changeSince(base, directory);
        const shown = [...change.added].map(([path, lines]) => `${path}: ${lines === null ? 'all' : [...lines].join(', ')}`).sort();
        assert.deepEqual(shown, ['kept.md: 2, 5', 'loose.md: all', 'naïve name.md: 2', 'staged.md: 1, 2']);
        assert.deepEqual([...change.newFiles].sort(), ['loose.md', 'staged.md']);
    });
});

test('diff reads a change whatever the user has configured git to print', () => {
    withRepository((directory, base) => {
        run(directory, 'config', 'diff.noprefix', 'true');
        run(directory, 'config', 'color.diff', 'always');
        run(directory, 'config', 'diff.renames', 'copies');
        run(directory, 'config', 'diff.mnemonicPrefix', 'true');
        writeFileSync(join(directory, 'kept.md'), 'one\ntwo\nthree\nfour\n');
        const change = changeSince(base, directory);
        assert.deepEqual([...change.added].map(([path, lines]) => `${path}: ${[...(lines ?? [])].join(', ')}`), ['kept.md: 4']);
    });
});
