/**
 * Tests for scripts/lexer.ts, the C++ lexer the review gates share.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { blankComments, codeLines, commentLines, isDigitSeparator, lex } from '../lexer.ts';

/** The comment bodies of the text, as [line number, body] pairs. */
function comments(text: string): Array<[number, string]> {
    const out: Array<[number, string]> = [];
    commentLines(text).forEach((bodies, index) => {
        for (const { text: body } of bodies) {
            out.push([index + 1, body]);
        }
    });
    return out;
}

test('a comment marker inside a string or character literal starts no comment', () => {
    const text = 's = "a // b /* c"; t = \'//\'; u = \'/*\'; // real';
    assert.deepEqual(codeLines(text), ['s = ""; t = \'\'; u = \'\'; ']);
    assert.deepEqual(comments(text), [[1, ' real']]);
});

test('an escaped quote does not end its literal', () => {
    const text = 'a = \'"\'; b = \'\\\'\'; c = "\\" // no"; // yes';
    assert.deepEqual(codeLines(text), ['a = \'\'; b = \'\'; c = ""; ']);
    assert.deepEqual(comments(text), [[1, ' yes']]);
});

test('a digit separator is code and opens no literal', () => {
    for (const number of ['1\'000', '0x1\'F', '0b1\'0', '1.5\'0', '1\'000\'000']) {
        const text = `int n = ${number};  // So it ends here.`;
        assert.deepEqual(codeLines(text), [`int n = ${number};  `], number);
        assert.deepEqual(comments(text), [[1, ' So it ends here.']], number);
    }
    assert.equal(isDigitSeparator('1\'000', 1), true);
    assert.equal(isDigitSeparator('0x1\'F', 3), true);
});

test('a quote after a prefix or a letter opens a character literal', () => {
    const text = 'auto a = u8\'a\'; auto b = \'x\'; auto c = L\'"\'; // c';
    assert.deepEqual(codeLines(text), ['auto a = u8\'\'; auto b = \'\'; auto c = L\'\'; ']);
    assert.deepEqual(comments(text), [[1, ' c']]);
    assert.equal(isDigitSeparator('u8\'a\'', 2), false);
    assert.equal(isDigitSeparator('\'x\'', 0), false);
});

test('a raw string ends only at its own delimiter', () => {
    assert.deepEqual(codeLines('x = R"(a // b " c)"; // d'), ['x = ""; ']);
    assert.deepEqual(codeLines('x = R"d(a )" // b )d"; // c'), ['x = ""; ']);
    assert.deepEqual(comments('x = R"d(a )" // b )d"; // c'), [[1, ' c']]);
});

test('a raw string takes the u8, L, u and U prefixes', () => {
    for (const prefix of ['u8R', 'LR', 'uR', 'UR']) {
        const text = `x = ${prefix}"(" // not)"; // yes`;
        assert.deepEqual(codeLines(text), ['x = ""; '], prefix);
        assert.deepEqual(comments(text), [[1, ' yes']], prefix);
    }
});

test('an identifier ending in R before a quote is not a raw string prefix', () => {
    const text = 'fooR"(" // c';
    assert.deepEqual(codeLines(text), ['fooR"" ']);
    assert.deepEqual(comments(text), [[1, ' c']]);
});

test('a raw string across lines hides the comment markers inside it', () => {
    const text = [
        'auto s = R"(',
        '// not a comment',
        '/* nor this',
        ')";  // real',
        'int x; /* also real */',
    ].join('\n');
    assert.deepEqual(codeLines(text), ['auto s = ""', '', '', ';  ', 'int x;  ']);
    assert.deepEqual(comments(text), [[4, ' real'], [5, ' also real ']]);
});

test('a backslash before LF continues a literal onto the next line', () => {
    const text = 's = "a \\\n// b /* c";  // real\nint y;';
    assert.deepEqual(codeLines(text), ['s = ""', ';  ', 'int y;']);
    assert.deepEqual(comments(text), [[2, ' real']]);
});

test('a backslash before CRLF continues a literal onto the next line', () => {
    const text = 's = "a \\\r\n// b /* c";  // real\r\nint y;\r\n';
    assert.deepEqual(codeLines(text), ['s = ""', ';  ', 'int y;', '']);
    assert.deepEqual(comments(text), [[2, ' real']]);
});

test('a literal with no closing quote ends at its line break', () => {
    const text = '#error don\'t\n// real';
    assert.deepEqual(codeLines(text), ['#error don\'\'', '']);
    assert.deepEqual(comments(text), [[2, ' real']]);
});

test('a block comment covers the lines after the one that opens it', () => {
    const text = 'int a; /* open\n*out = 1;\n\n*/ int b;';
    assert.deepEqual(codeLines(text), ['int a;  ', '', '', ' int b;']);
    assert.deepEqual(comments(text), [[1, ' open'], [2, '*out = 1;'], [3, ''], [4, '']]);
});

test('a comment body keeps its column', () => {
    const line = 'int a;  // body';
    assert.deepEqual(commentLines(line), [[{ column: line.indexOf('//') + 2, text: ' body' }]]);
    const block = 'f(); /* one */ g(); /* two';
    assert.deepEqual(commentLines(block), [[
        { column: block.indexOf('/*') + 2, text: ' one ' },
        { column: block.lastIndexOf('/*') + 2, text: ' two' },
    ]]);
});

test('every result has one entry per line, and a line keeps its number', () => {
    const text = [
        'auto r = R"x(',     // 1
        ')" no )x";',        // 2
        's = "a \\',         // 3
        'b";',               // 4
        '/* one',            // 5
        'two */',            // 6
        'c = \'\\\'\';',     // 7
        'marker();  // m',   // 8
    ].join('\r\n');
    const count = text.split('\n').length;
    assert.equal(lex(text).length, count);
    assert.equal(codeLines(text).length, count);
    assert.equal(commentLines(text).length, count);
    assert.equal(codeLines(text)[7], 'marker();  ');
    assert.deepEqual(comments(text).at(-1), [8, ' m']);
});

test('lex marks where each token opens and closes', () => {
    const lines = lex('a /* b\nc */ d');
    assert.deepEqual(lines[0], [
        { kind: 'code', column: 0, text: 'a ', opens: true, closes: true },
        { kind: 'comment', column: 2, text: '/* b', opens: true, closes: false },
    ]);
    assert.deepEqual(lines[1], [
        { kind: 'comment', column: 0, text: 'c */', opens: false, closes: true },
        { kind: 'code', column: 4, text: ' d', opens: true, closes: true },
    ]);
});

test('blankComments keeps every offset and leaves literals alone', () => {
    const text = 'a = "/* x */"; /* y\r\nz */ b = R"(//)"; // w\nc';
    const blanked = blankComments(text);
    assert.equal(blanked.length, text.length);
    assert.equal(blanked, 'a = "/* x */";      \n     b = R"(//)";     \nc');
});
