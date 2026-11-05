/**
 * Tests for scripts/lib/script.ts, which finds the comments and literals of a TypeScript file.
 *
 * Each test feeds the scanner a shape a simpler reading gets wrong, and checks what is left as
 * code. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { codeOnly, regions } from '../lib/script.ts';

/** The code of a text with the blanked runs squeezed to one space, so a test reads as code. */
function code(text: string): string {
    return codeOnly(text).replace(/ +/g, ' ');
}

test('script keeps the length and the line breaks of the text', () => {
    const text = "const a = 'x'; // note\n/* two\nlines */ const b = `t${a}t`;\n";
    const result = codeOnly(text);
    assert.equal(result.length, text.length);
    assert.deepEqual([...result].map((c, i) => (c === '\n') === (text[i] === '\n')).every(Boolean), true);
});

test('script blanks comments and the insides of strings, and keeps their quotes', () => {
    assert.equal(code("a('x // y'); // gone"), "a(' '); ");
    assert.equal(code('a("q\\"/*"); /* gone */ b'), 'a(" "); b');
});

test('script reads a template expression as code, and its text as a literal', () => {
    assert.equal(code('`text ${f(\'s\')} more`'), "` ${f(' ')} `");
    // a brace inside the expression does not end it
    assert.equal(code('`a${ {k: 1}.k }b`'), '` ${ {k: 1}.k } `');
    // a template inside an expression
    assert.equal(code('`a${`inner ${x}`}b`'), '` ${` ${x}`} `');
});

test('script tells a regular expression from a division', () => {
    assert.equal(code("const r = /'[/]x/g; f('s')"), "const r = / /g; f(' ')");
    assert.equal(code('const half = total / 2 / count; // c'), 'const half = total / 2 / count; ');
    assert.equal(code('return /a\\/b/.test(s)'), 'return / /.test(s)');
    assert.equal(code('x = (a) / b'), 'x = (a) / b');
    // a slash after a closing bracket divides, even with another slash later on the line
    assert.equal(code("x = (a) / b / c('s')"), "x = (a) / b / c(' ')");
    assert.equal(code("x = a[0] / b / c('s')"), "x = a[0] / b / c(' ')");
});

test('script reads an escaped backtick as part of the template', () => {
    assert.equal(code('`a\\`b` + x'), '` ` + x');
    assert.equal(code("`a\\${b}` + f('s')"), "` ` + f(' ')");
});

test('script ends an unterminated string at the line, and an unterminated comment at the end', () => {
    assert.equal(code("a = 'open\nb = 1"), "a = ' \nb = 1");
    assert.deepEqual(regions('x /* never closed').map((r) => r.kind), ['comment']);
});

test('script reports each region with its kind, in order', () => {
    const text = "f('a') // c\n`t`";
    assert.deepEqual(regions(text).map((r) => [r.kind, text.slice(r.start, r.end)]),
        [['literal', 'a'], ['comment', '// c'], ['literal', 't']]);
});
