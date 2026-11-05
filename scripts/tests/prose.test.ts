/**
 * Tests for the prose gate in scripts/prose.ts and scripts/prose/.
 *
 * The first tests feed the gate shapes of C++ that a lexer can misread. The rest check each
 * writing rule on Markdown and on comments, the structure the gate does not read, and the paths it
 * leaves out. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { checkLines } from '../prose.ts';
import { excluded, fileKind } from '../prose/files.ts';
import { scriptSpans } from '../prose/paragraphs.ts';

/** The prose findings for the lines of a C++ file, as "line: rule" strings. */
function prose(lines: string[]): string[] {
    return checkLines('api/example/Example.cxx', lines, null).map((f) => `${f.number}: ${f.rule}`);
}

/** A sentence of the given number of words and its ending, wrapped ten words to a line. */
function wrapped(count: number, ending: string): string[] {
    const words = Array.from({ length: count }, (_, i) => `w${i}`);
    const lines: string[] = [];
    for (let at = 0; at < words.length; at += 10) {
        lines.push(words.slice(at, at + 10).join(' '));
    }
    lines[lines.length - 1] += ending;
    return lines;
}

/** The findings for the lines of a Markdown file at a path, as "line: rule" strings. */
function markdown(lines: string[], path = 'docs/Example.md', added: Set<number> | null = null): string[] {
    return checkLines(path, lines, added).map((f) => `${f.number}: ${f.rule}`);
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

test('prose reports a comment line past 100 columns, and not a comment after code', () => {
    const long = `// ${'word '.repeat(20)}`;
    const lines = [
        '// A short comment line before it.',
        long,
        `int count = 0;  ${long}`,
    ];
    assert.deepEqual(prose(lines), ['2: long-line']);
});

test('prose reports a sentence with three clause joins, and not one with two', () => {
    const lines = [
        '// The ring is shared, which keeps it small, so a frame waits because the fence is set.',
        '',
        '// The ring is shared, so a frame waits because the fence is set.',
    ];
    assert.deepEqual(prose(lines), ['1: stacked']);
});

test('prose reports two clause joins when one is a semicolon or a dash', () => {
    const lines = [
        '// The ring is shared; a frame waits because the fence is set.',
        '',
        '// The ring is shared - a frame waits, so the fence is set.',
        '',
        '// The ring is shared by every frame, so a frame waits because the fence is set.',
    ];
    assert.deepEqual(prose(lines), ['1: stacked', '3: stacked']);
});

test('prose counts the dashes around an aside as one join, and not a minus sign', () => {
    const lines = [
        '// Every quad - a sprite or a glyph - is drawn once.',
        '',
        '// The count is n - 1, so the last index is n - 1 too.',
        '',
        '// Every quad - a sprite or a glyph - is drawn once, so the count is small.',
    ];
    assert.deepEqual(prose(lines), ['5: stacked']);
});

test('prose reports two relative clauses on one noun', () => {
    const lines = [
        '// The frames are a Ring, which this drives but which works without it.',
        '',
        '// The engine registers quit, which ends the loop, and pause, which stops it.',
    ];
    assert.deepEqual(prose(lines), ['1: stacked']);
});

test('prose does not count a semicolon that ends a list item', () => {
    const lines = [
        '// - the factory, which copies through the uploader;',
        '//',
        '// The factory, which copies through the uploader; the pool is shared.',
    ];
    assert.deepEqual(prose(lines), ['3: stacked']);
});

test('prose reports "X is what" followed by a verb, and not by a subject', () => {
    const lines = [
        '// The submit made by present() is what signals the fence.',
        '',
        '// The pixels are what the file holds, and the offset is what insets() returns.',
    ];
    assert.deepEqual(prose(lines), ['1: inverted']);
});


test('prose reports a sentence of more than 35 words, and counts a code span as one word', () => {
    assert.deepEqual(markdown(wrapped(36, '.')), ['1: long-sentence']);
    assert.deepEqual(markdown(wrapped(35, '.')), []);
    assert.deepEqual(markdown(wrapped(34, ' `a span of six words in it`.')), []);
});

test('prose divides sentences at a full stop, and not after an abbreviation', () => {
    assert.deepEqual(markdown([...wrapped(20, '.'), ...wrapped(20, '.')]), []);
    assert.deepEqual(markdown([...wrapped(20, ', e.g.'), ...wrapped(20, '.')]), ['1: long-sentence']);
});

test('prose reports a sentence that opens with And or So, and not a word that starts with them', () => {
    assert.deepEqual(markdown(['The ring is shared. And a frame waits.']), ['1: opener']);
    assert.deepEqual(markdown(['So the frame waits.']), ['1: opener']);
    assert.deepEqual(markdown(['Sorting is stable. Android is not a word here.']), []);
});

test('prose reports "X, which is what Y" and its kin', () => {
    assert.deepEqual(markdown(['The fence is set, which is why the frame waits.']), ['1: inverted']);
    assert.deepEqual(markdown(['The fence is set, which is the reason the frame waits.']), []);
});

test('prose reports a thing that knows or wants, and not a person, an app or a caller', () => {
    assert.deepEqual(markdown(['The renderer knows the size.']), ['1: personification']);
    assert.deepEqual(markdown(['The pool does not know the frame.']), ['1: personification']);
    assert.deepEqual(markdown(['The buffer that wants a copy is read.']), ['1: personification']);
    assert.deepEqual(markdown(['The user knows the size. An app that wants a camera asks. A caller owes nothing.']), []);
    assert.deepEqual(markdown(['Whoever writes the shader wants it fast.']), []);
});

test('prose reports history: "used to", a date and a phase, but not the passive "is used to"', () => {
    assert.deepEqual(markdown(['The ring used to hold three frames.']), ['1: history']);
    assert.deepEqual(markdown(['The ring is used to hold three frames.']), []);
    assert.deepEqual(markdown(['The ring changed on 2026-11-02.']), ['1: history']);
    assert.deepEqual(markdown(['The ring changed in March 3, 2026.']), ['1: history']);
    assert.deepEqual(markdown(['The ring arrives in phase 2.']), ['1: history']);
});

test('prose lets plans and roadmaps describe change, and a document that names its phases refer to them', () => {
    const lines = ['The ring used to hold three frames, and phase 2 changes it on 2026-11-02.'];
    assert.deepEqual(markdown(lines, 'docs/plans/Ring.md'), []);
    assert.deepEqual(markdown(lines, 'docs/roadmap/Ring.md'), []);
    assert.deepEqual(markdown(['# Phase 1: set up', '', 'Run phase 1 first.']), []);
});

test('prose reports a Markdown line past 100 columns, unless a link cannot wrap', () => {
    const long = 'word '.repeat(21).trim();
    assert.deepEqual(markdown([long]), ['1: long-line']);
    assert.deepEqual(markdown([`- [A title](https://example.com/${'x'.repeat(100)})`]), []);
    assert.deepEqual(markdown([`https://example.com/${'x'.repeat(100)}`]), []);
    assert.deepEqual(markdown([`<https://example.com/${'x'.repeat(100)}>.`]), []);
    assert.deepEqual(markdown([`Read [the guide](${'x/'.repeat(50)}guide.md).`]), []);
    assert.deepEqual(markdown([`See ${'x'.repeat(60)} and [a link](a.md) then ${'y'.repeat(40)}.`]), ['1: long-line']);
});

test('prose does not read code fences, tables, headings, comments, metadata or front matter', () => {
    const so = 'So this would be reported as an opener.';
    const long = 'word '.repeat(21).trim();
    const lines = [
        '---',
        so,
        long,
        '---',
        `# ${so}`,
        '```',
        so,
        long,
        '```',
        `| ${so} | x |`,
        `<!-- ${so}`,
        so,
        '-->',
        `**Status**: accepted. ${so}`,
        so,
    ];
    assert.deepEqual(markdown(lines), ['15: opener']);
});

test('prose starts a paragraph at each list item, so items do not join into one sentence', () => {
    const words = Array.from({ length: 20 }, (_, i) => `w${i}`).join(' ');
    assert.deepEqual(markdown([`- ${words}`, `- ${words}`]), []);
    assert.deepEqual(markdown([words, `${words}.`]), ['1: long-sentence']);
});

test('prose checks only the sentences that overlap an added line', () => {
    const lines = ['So the first sentence is old.', '', 'So the second one is new.'];
    assert.deepEqual(markdown(lines, 'docs/Example.md', new Set([3])), ['3: opener']);
    assert.deepEqual(markdown(lines, 'docs/Example.md', new Set()), []);
    // a sentence that starts on an old line and runs onto an added one is checked
    assert.deepEqual(markdown(['So it starts here', 'and ends here.'], 'docs/Example.md', new Set([2])), ['1: opener']);
});

test('prose reports a comment line that runs past the rest of its paragraph', () => {
    const body = 'x'.repeat(70);
    const lines = [`// ${body}`, `// ${body}`, `// ${body} ${'y'.repeat(12)}`, `// ${body}`];
    assert.deepEqual(prose(lines), ['3: extended-line']);
    // a paragraph wrapped short is not held to its own width
    assert.deepEqual(prose(['// short', '// short', `// ${'z'.repeat(40)}`]), []);
    // nor is a paragraph of two lines, or a line that holds a URL
    assert.deepEqual(prose([`// ${body}`, `// ${body} ${'y'.repeat(12)}`]), []);
    assert.deepEqual(prose([`// ${body}`, `// ${body}`, `// ${body} https://e.co/${'y'.repeat(4)}`, `// ${body}`]), []);
});

test('prose does not report a long comment line that holds a URL', () => {
    assert.deepEqual(prose([`// See https://example.com/${'x'.repeat(100)}`]), []);
});

test('prose reads the comments of each kind of file it checks', () => {
    assert.deepEqual(markdown(['# So a comment line.', 'set(X 1)  # So a trailing comment.'], 'cmake/x.cmake'), ['1: opener', '2: opener']);
    assert.deepEqual(markdown(['rem So a remark.', ':: So another.', 'echo So not a comment.'], 'scripts/x.cmd'), ['1: opener', '2: opener']);
    assert.deepEqual(markdown(['# So a comment.', 'key: So a value.'], '.github/workflows/x.yml'), ['1: opener']);
    assert.deepEqual(markdown(["const s = '// So inside a string.'; // So a comment."], 'scripts/x.ts'), ['1: opener']);
    assert.deepEqual(markdown(['So nothing here is read.'], 'data/notes.txt'), []);
});

test('prose names the kind of a file by its name', () => {
    assert.equal(fileKind('docs/README.md'), 'md');
    assert.equal(fileKind('api/ui/Screen.cpp'), 'slash');
    assert.equal(fileKind('shaders/lit.FRAG'), 'slash');
    assert.equal(fileKind('scripts/prose.ts'), 'script');
    assert.equal(fileKind('api/CMakeLists.txt'), 'hash');
    assert.equal(fileKind('scripts/build.cmd'), 'cmd');
    assert.equal(fileKind('scripts/OLD.BAT'), 'cmd');
    assert.equal(fileKind('notes.txt'), null);
});

test('prose leaves out records, third-party trees and an ADR that is not new', () => {
    for (const path of ['vendor/x/README.md', 'ports/glm/usage.md', 'docs/audits/x.md', 'docs/plans/completed/X.md',
        'docs/roadmap/completed/X.md', 'out/build/x.md', 'examples/starter/vcpkg_installed/x.md']) {
        assert.equal(excluded(path, new Set()), true, path);
    }
    assert.equal(excluded('docs/adr/0001-a-decision.md', new Set()), true);
    assert.equal(excluded('docs/adr/0001-a-decision.md', new Set(['docs/adr/0001-a-decision.md'])), false);
    assert.equal(excluded('docs/adr/README.md', new Set()), false);
    assert.equal(excluded('docs/plans/Open.md', new Set()), false);
});

test('prose reads the comments of TypeScript without its strings, templates or regular expressions', () => {
    const spans = (text: string) => scriptSpans(text.split('\n')).map((s) => `${s.number}:${s.text}`);
    assert.deepEqual(spans("const r = /'[/]x/g; // after\nconst h = a / 2; // half"), ['1: after', '2: half']);
    assert.deepEqual(spans('const t = `${`/* no */`}`; /* yes */'), ['1: yes ']);
    assert.deepEqual(spans('/**\n * one\n */'), ['1:*', '2: * one', '3: ']);
});
