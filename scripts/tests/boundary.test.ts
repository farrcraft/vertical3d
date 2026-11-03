/**
 * Tests for scripts/boundary.ts, the check for boundary input converted without a check.
 *
 * Each test feeds the gate the added lines of one C++ file, in a shape a lexer can misread or a
 * shape each rule has to see. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { attachCode, check } from '../boundary.ts';

/** The boundary reports for the given lines of one C++ file, as "line: rule" strings. */
function boundary(text: string, numbers: number[], path = 'api/example/Example.cxx'): string[] {
    const lines = text.split('\n');
    const added = numbers.map((number) => ({ path, number, text: lines[number - 1] ?? '' }));
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

test('boundary reports any value_to, and a cast whose operand gives a float', () => {
    const text = [
        'auto n = boost::json::value_to<int>(v);',
        'int a = static_cast<int>(std::round(x));',
        'auto b = static_cast<std::size_t>(v.as_double());',
        'int c = static_cast<int>(count.value());',
        'int d = static_cast<int>(2.5f);',
        'int e = static_cast<int>(static_cast<float>(y));',
    ].join('\n');
    assert.deepEqual(boundary(text, [1, 2, 3, 4, 5, 6]),
        ['1: value-to', '2: float-cast', '3: float-cast', '4: float-cast', '5: float-cast', '6: float-cast']);
});

test('boundary does not report a cast of an integer, or a cast to a float', () => {
    const text = 'int a = static_cast<int>(size);\nfloat b = static_cast<float>(std::floor(x));\n';
    assert.deepEqual(boundary(text, [1, 2]), []);
});

test('boundary accepts a line whose trailing comment names why the conversion is defined', () => {
    const text = 'int a = static_cast<int>(std::fmod(x, 256.0f)) & 255;  // checked: x is finite\n';
    assert.deepEqual(boundary(text, [1]), []);
    // the comment has to say something after checked:
    assert.deepEqual(boundary('int a = static_cast<int>(std::floor(x));  // checked:\n', [1]), ['1: float-cast']);
});

test('boundary reads only the added lines it is given', () => {
    const text = 'int a = static_cast<int>(std::floor(x));\nint b = static_cast<int>(std::floor(y));\n';
    assert.deepEqual(boundary(text, [2]), ['2: float-cast']);
});

test('boundary leaves out tests, the checked helpers, third-party code and other files', () => {
    const cast = 'int a = static_cast<int>(std::floor(x));\n';
    for (const path of ['api/type/tests/CheckedTest.cxx', 'api/type/Checked.h', 'api/asset/Json.cpp', 'vendor/x/a.cpp',
        'demos/voxel/src/noise/a.cpp', 'docs/a.md']) {
        assert.deepEqual(boundary(cast, [1], path), [], path);
    }
    assert.deepEqual(boundary(cast, [1], 'demos/voxel/src/World.cpp'), ['1: float-cast']);
});
