/**
 * Tests for the entry points of the review gates that read C++ through scripts/lexer.ts.
 *
 * Each test feeds a gate a shape of C++ source that a lexer can misread. Run them with
 * node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import process from 'node:process';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { attachCode, check } from '../boundary.ts';
import { casesIn } from '../failsfirst.ts';
import { includesIn, usesIn } from '../linkrule.ts';
import { checkLines } from '../prose.ts';

/** The prose findings for the lines of a C++ file, as "line: rule" strings. */
function prose(lines: string[]): string[] {
    return checkLines('api/example/Example.cxx', lines, null).map((f) => `${f.number}: ${f.rule}`);
}

/** The boundary reports for the given lines of one C++ file, as "line: rule" strings. */
function boundary(text: string, numbers: number[]): string[] {
    const path = 'api/example/Example.cxx';
    const lines = text.split('\n');
    const added = numbers.map((number) => ({ path, number, text: lines[number - 1] }));
    return check(attachCode(added, () => text)).map((r) => `${r.number}: ${r.rule}`);
}

test('prose reads a comment that follows a digit separator', () => {
    assert.deepEqual(prose(['int count = 1\'000;  // So the count is large.']), ['1: opener']);
});

test('prose skips a comment marker inside a raw string and reads the comment after it', () => {
    const lines = [
        'const char* text = R"(',
        '// So this line is inside a raw string.',
        ')";',
        '',
        '// So this one is a real comment.',
    ];
    assert.deepEqual(prose(lines), ['5: opener']);
});

test('prose skips a comment marker inside a continued string', () => {
    const lines = [
        'const char* text = "one \\',
        '// So this line is inside the string."',
        '    "and so is this one.";',
        '// So this one is a real comment.',
    ];
    assert.deepEqual(prose(lines), ['4: opener']);
});

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

test('failsfirst finds a case after a CRLF-continued string and after a raw string', () => {
    const text = [
        'BOOST_AUTO_TEST_SUITE(suite)',
        'const char* a = "one \\',
        'two /* not a comment";',
        'BOOST_AUTO_TEST_CASE(after_continued) {}',
        'const char* b = R"(" /* )";',
        'BOOST_AUTO_TEST_CASE(after_raw) {}',
        '/* a real comment */',
        'BOOST_AUTO_TEST_SUITE_END()',
        '',
    ].join('\r\n');
    assert.deepEqual(casesIn(text).map((c) => c.path), ['suite/after_continued', 'suite/after_raw']);
});

test('failsfirst skips a case inside a comment', () => {
    const text = '// BOOST_AUTO_TEST_CASE(commented) {}\n/* BOOST_AUTO_TEST_CASE(blocked) {} */\nBOOST_AUTO_TEST_CASE(real) {}\n';
    assert.deepEqual(casesIn(text).map((c) => c.path), ['real']);
});

const GRID = new Map([['grid', new Set(['v3dlib_grid'])]]);

test('linkrule counts a using-directive and not a namespace declaration', () => {
    assert.deepEqual(usesIn('using namespace v3d::grid;', GRID), [[['v3dlib_grid'], 'v3d::grid']]);
    assert.deepEqual(usesIn('namespace v3d::grid {\n}', GRID), []);
});

test('linkrule does not count a name inside a comment or a literal', () => {
    const text = '// v3d::grid::Cell\nconst char* s = "v3d::grid::Cell";\nauto r = u8R"(v3d::grid::Cell)";\n';
    assert.deepEqual(usesIn(text, GRID), []);
});

test('linkrule counts a name after a character literal holding a quote', () => {
    assert.deepEqual(usesIn('auto c = u8\'"\'; v3d::grid::Cell cell;', GRID), [[['v3dlib_grid'], 'v3d::grid::Cell']]);
});

test('linkrule counts a name after a CRLF-continued string, and not one inside it', () => {
    // the continued part holds a qualified name; read as code it would be counted as a use
    const text = 'const char* s = "a \\\r\nv3d::grid::Other";\r\nv3d::grid::Cell cell;\r\n';
    assert.deepEqual(usesIn(text, GRID), [[['v3dlib_grid'], 'v3d::grid::Cell']]);
});

test('linkrule reads includes outside comments and #if 0 blocks', () => {
    const text = [
        '#include "a.h"  // #include "b.h"',
        '/* #include "c.h"',
        '#include "d.h" */',
        '#if 0',
        '#include "e.h"',
        '#endif',
        '#include <f.h>',
    ].join('\n');
    assert.deepEqual(includesIn(text), ['a.h', 'f.h']);
});

// Each gate runs its command line when Node is started with it, and not when a test imports it.
// A gate that did not run would exit 0, so each is given a base that is not a commit and must
// refuse it.
for (const gate of ['prose.ts', 'boundary.ts']) {
    test(`${gate} runs when Node is started with it`, () => {
        const script = fileURLToPath(new URL(`../${gate}`, import.meta.url));
        const run = spawnSync(process.execPath, [script, '--base', 'no-such-ref'], { encoding: 'utf8' });
        assert.equal(run.status, 2, run.stderr);
        assert.match(run.stderr, /no-such-ref/);
    });
}

test('failsfirst.ts runs when Node is started with it', () => {
    const script = fileURLToPath(new URL('../failsfirst.ts', import.meta.url));
    const run = spawnSync(process.execPath, [script, '--no-such-option'], { encoding: 'utf8' });
    assert.equal(run.status, 2, run.stderr);
});

test('linkrule.ts runs when Node is started with it', () => {
    const script = fileURLToPath(new URL('../linkrule.ts', import.meta.url));
    const run = spawnSync(process.execPath, [script, '--no-such-option'], { encoding: 'utf8' });
    assert.equal(run.status, 2, run.stderr);
    assert.match(run.stderr, /usage/);
});
