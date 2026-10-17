/**
 * Tests for scripts/commentsonly.ts, the check that a change moved nothing but comments.
 *
 * Each test compares two versions of one file. The ones that pass rewrite only comments or
 * whitespace, and the ones that fail change code in a way a comment could hide. Run them with
 * node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { join } from 'node:path';
import process from 'node:process';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { compare, syntax } from '../commentsonly.ts';

const ROOT = fileURLToPath(new URL('../..', import.meta.url));

/** The findings for a change to a file, as "line: rule" strings. */
function findings(path: string, was: string[], now: string[]): string[] {
    return compare(path, was.join('\n'), now.join('\n')).map((f) => `${f.number}: ${f.rule}`);
}

test('commentsonly passes a reworded and reflowed docblock', () => {
    const was = [
        '/**',
        ' * The width, which is what the layout reads.',
        ' */',
        'int width() const;',
    ];
    const now = [
        '/**',
        ' * The width in pixels. The layout reads it',
        ' * on every pass.',
        ' */',
        'int width() const;',
    ];
    assert.deepEqual(findings('api/ui/Example.h', was, now), []);
});

test('commentsonly passes a removed trailing comment and a removed comment line', () => {
    const was = ['// narrates the next line', 'int x = 1;  // one', '', 'int y = 2;'];
    const now = ['int x = 1;', 'int y = 2;'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), []);
});

test('commentsonly reports a changed value beside an unchanged comment', () => {
    const was = ['// the default', 'int x = 1;'];
    const now = ['// the default', 'int x = 2;'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), ['2: code-changed']);
});

test('commentsonly reports a changed string literal, whitespace included', () => {
    const was = ['const char* text = "a b";'];
    const now = ['const char* text = "a  b";'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), ['1: code-changed']);
});

test('commentsonly reads a comment marker inside a string as code', () => {
    const was = ['const char* url = "http://example";'];
    const now = ['const char* url = "http://other";'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), ['1: code-changed']);
});

test('commentsonly reports code joined by removing a block comment', () => {
    const was = ['int/**/x = 1;'];
    const now = ['intx = 1;'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), ['1: code-changed']);
});

test('commentsonly reports a line comment that swallows the next line by a continuation', () => {
    const was = ['#define A 1  // one', 'int x = A;'];
    const now = ['#define A 1  // one \\', 'int x = A;'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), ['1: continued']);
    assert.deepEqual(findings('api/example/Example.cxx', now, now), []);
});

test('commentsonly reports a block comment that closes early', () => {
    const was = ['/* a comment */', 'int x = 1;'];
    const now = ['/* a comment */ that went on */', 'int x = 1;'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), ['1: code-changed', '1: stray-close']);
});

test('commentsonly reports code moved onto another line', () => {
    const was = ['#define A 1', 'int x = A;'];
    const now = ['#define A 1 int x = A;'];
    assert.deepEqual(findings('api/example/Example.cxx', was, now), ['1: code-changed']);
});

test('commentsonly passes a CMake comment that changed, and keeps a # inside quotes', () => {
    const was = ['# the sources', 'set(SOURCES a.cpp)  # one', 'message("#1")'];
    const now = ['# the sources this target builds', 'set(SOURCES a.cpp)', 'message("#1")'];
    assert.deepEqual(findings('api/example/CMakeLists.txt', was, now), []);
    assert.deepEqual(findings('api/example/CMakeLists.txt', ['message("#1")'], ['message("#2")']),
        ['1: code-changed']);
});

test('commentsonly passes a changed CMake bracket comment and counts the lines after it', () => {
    const was = ['#[[ one', 'two ]]', 'set(A 1)'];
    const now = ['#[==[ one', 'two', 'three ]==]', 'set(A 2)'];
    assert.deepEqual(findings('cmake/v3dHelpers.cmake', was, now), ['4: code-changed']);
});

test('commentsonly reads C++, GLSL and CMake files and nothing else', () => {
    assert.equal(syntax('api/ui/Engine.h'), 'slash');
    assert.equal(syntax('api/render/realtime/shaders/mesh.frag'), 'slash');
    assert.equal(syntax('CMakeLists.txt'), 'hash');
    assert.equal(syntax('pong/CMakeLists.txt'), 'hash');
    assert.equal(syntax('cmake/v3dHelpers.cmake'), 'hash');
    assert.equal(syntax('docs/README.md'), null);
    assert.equal(syntax('scripts/prose.ts'), null);
    assert.equal(syntax('notes.txt'), null);
});

test('commentsonly.ts runs when Node is started with it', () => {
    const run = spawnSync(process.execPath, [join(ROOT, 'scripts', 'commentsonly.ts'), '--base', 'no-such-ref'],
        { cwd: ROOT, encoding: 'utf8' });
    assert.equal(run.status, 2, run.stderr);
    assert.match(run.stderr, /no-such-ref/);
});
