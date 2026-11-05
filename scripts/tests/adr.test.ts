/**
 * Tests for scripts/adr.ts, the check of the decision records and the indexes made from them.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import {
    END, indexPart, parseRecord, problems, type Record, removedNumbers, splice, spliceSummary, START, summaryEntries, tagOf, tagPart,
    vocabulary,
} from '../adr.ts';

/** A record's text, with header lines added after its status. */
function text(number: string, title: string, status: string, lines: string[] = []): string {
    return [`# ADR-${number}: ${title}`, '', `**Status**: ${status}`, '**Date**: 2026-11-04', ...lines,
        '**Documented in**: a page', '', '## Context', '', '**Status**: not a header line'].join('\n');
}

/** A record in a folder, read from its text. */
function record(folder: string, number: string, title: string, status: string, lines: string[] = []): Record {
    const slug = title.toLowerCase().replace(/[^a-z0-9]+/g, '-');
    return parseRecord(`docs/adr/${folder}/${number}-${slug}.md`, text(number, title, status, lines));
}

const TAGS = new Map([['rendering', 'the renderer'], ['editor', 'the editor'], ['vulkan', 'Vulkan']]);

/** The rules of the findings for some records, in order. */
function rules(records: Record[], removed: number[] = []): string[] {
    return problems(records, TAGS, new Set(removed)).map((finding) => finding.rule);
}

test('adr reads a record from its title and the header lines before its first section', () => {
    const read = record('rendering', '0012', 'Rendering: one thing', 'amended',
        ['**Tags**: rendering, vulkan', '**Amended by**: [ADR-0013](0013-a.md), ADR-0014']);
    assert.equal(read.folder, 'rendering');
    assert.equal(read.number, 12);
    assert.equal(read.title, 'Rendering: one thing');
    assert.equal(read.area, 'Rendering');
    assert.equal(read.status, 'amended');
    assert.deepEqual(read.tags, ['rendering', 'vulkan']);
    assert.deepEqual(read.amendedBy, [13, 14]);
    assert.deepEqual(read.amends, []);
});

test('adr makes a tag of a title area word in lower case, with dashes for spaces', () => {
    assert.equal(tagOf('UI'), 'ui');
    assert.equal(tagOf('Lit scenes'), 'lit-scenes');
});

test('adr accepts records that are filed, tagged and linked both ways', () => {
    const amended = record('rendering', '0001', 'Rendering: first', 'amended', ['**Tags**: rendering', '**Amended by**: ADR-0002']);
    const amending = record('editor', '0002', 'Editor: second', 'accepted', ['**Tags**: editor, vulkan', '**Amends**: ADR-0001']);
    assert.deepEqual(rules([amended, amending]), []);
});

test('adr reports a record outside an area folder, and a number that is taken or removed', () => {
    assert.deepEqual(rules([record('nowhere', '0001', 'Rendering: a', 'accepted', ['**Tags**: rendering, editor, vulkan'])]),
        ['misplaced']);
    const first = record('rendering', '0001', 'Rendering: a', 'accepted', ['**Tags**: rendering, editor, vulkan']);
    const second = record('rendering', '0001', 'Rendering: b', 'accepted', ['**Tags**: rendering']);
    assert.deepEqual(rules([first, second]), ['bad-number', 'bad-number']);
    assert.deepEqual(rules([first], [1]), ['bad-number']);
    const renumbered = parseRecord('docs/adr/rendering/0002-a.md', text('0001', 'Rendering: a', 'accepted', ['**Tags**: rendering, editor, vulkan']));
    assert.deepEqual(rules([renumbered]), ['bad-number']);
});

test('adr reports a title without an area, and a status the links do not give', () => {
    assert.deepEqual(rules([record('rendering', '0001', 'No area here', 'accepted', ['**Tags**: rendering, editor, vulkan'])]),
        ['bad-title']);
    assert.deepEqual(rules([record('rendering', '0001', 'Rendering: a', 'agreed', ['**Tags**: rendering, editor, vulkan'])]),
        ['bad-status']);
    assert.deepEqual(rules([record('rendering', '0001', 'Rendering: a', 'amended', ['**Tags**: rendering, editor, vulkan'])]),
        ['bad-status']);
});

test('adr reports a missing tag line, a tag the table lacks, a title area left untagged, and an unused tag', () => {
    assert.deepEqual(rules([record('rendering', '0001', 'Rendering: a', 'accepted')]), ['no-tags', 'unused-tag', 'unused-tag', 'unused-tag']);
    assert.deepEqual(rules([record('rendering', '0001', 'Rendering: a', 'accepted', ['**Tags**: rendering, editor, vulkan, sound'])]),
        ['unknown-tag']);
    assert.deepEqual(rules([record('rendering', '0001', 'Rendering: a', 'accepted', ['**Tags**: editor, vulkan'])]),
        ['untagged-area', 'unused-tag']);
});

test('adr reports a link to no record, and a link the other record does not answer', () => {
    const tags = '**Tags**: rendering, editor, vulkan';
    assert.deepEqual(rules([record('rendering', '0001', 'Rendering: a', 'accepted', [tags, '**Amends**: ADR-0009'])]),
        ['broken-link']);
    const amended = record('rendering', '0001', 'Rendering: a', 'amended', [tags]);
    const amending = record('rendering', '0002', 'Rendering: b', 'accepted', [tags, '**Amends**: ADR-0001']);
    assert.deepEqual(rules([amended, amending]), ['one-way']);
    const superseded = record('rendering', '0001', 'Rendering: a', 'superseded', [tags, '**Superseded by**: ADR-0002']);
    const unanswered = record('rendering', '0002', 'Rendering: b', 'accepted', [tags]);
    assert.deepEqual(rules([superseded, unanswered]), ['bad-status', 'one-way']);
});

test('adr reads the tag table, and the numbers that have no record', () => {
    assert.deepEqual([...vocabulary('| Tag | Covers |\n|---|---|\n| `ui` | `api/ui` |\n| `text` | fonts |\n').keys()], ['ui', 'text']);
    const readme = '## Numbers with no record\n\n| ADR | Where |\n|---|---|\n| 0006 | *gone* |\n\n## The index\n\n| [0007](a.md) | x |\n';
    assert.deepEqual([...removedNumbers(readme)], [6]);
});

test('adr indexes the records by area, in number order, with each status naming what changed it', () => {
    const amended = record('rendering', '0003', 'Rendering: a', 'amended', ['**Tags**: rendering', '**Amended by**: ADR-0002, ADR-0001']);
    const later = record('editor', '0002', 'Editor: b', 'accepted', ['**Tags**: editor', '**Amends**: ADR-0003']);
    const earlier = record('editor', '0001', 'Editor: c', 'accepted', ['**Tags**: editor', '**Amends**: ADR-0003']);
    const index = indexPart([later, amended, earlier], new Set([7]));
    assert.match(index, /^The next record is ADR-0008\./);
    assert.match(index, /\| \[0003\]\(rendering\/0003-rendering-a\.md\) \| Rendering: a \| amended by 0001, 0002 \| rendering \|/);
    assert.ok(index.indexOf('### Rendering') < index.indexOf('### Editor'));
    assert.ok(index.indexOf('[0001]') < index.indexOf('[0002]'));
    assert.match(tagPart([later, earlier], TAGS), /### editor\n\n\| ADR \| Decision \|\n\|---\|---\|\n\| \[0001\]\(editor\/0001-editor-c\.md\) \| Editor: c \|\n\| \[0002\]/);
    assert.match(index, /\| Rendering \| \[rendering\/\]\(rendering\/\) \| /);
});

test('adr replaces only the text between its markers, and only the entries under the index in the summary', () => {
    assert.equal(splice(`a\n${START}\nold\n${END}\nb`, 'new'), `a\n${START}\n\nnew\n\n${END}\nb`);
    assert.equal(splice('no markers', 'new'), null);
    const records = [record('editor', '0001', 'Editor: c', 'accepted', ['**Tags**: editor'])];
    const summary = '- [Docs](README.md)\n- [Architecture Decision Records](adr/README.md)\n    - [Old](adr/0001-c.md)\n- [Plans](plans/README.md)';
    const written = spliceSummary(summary, summaryEntries(records));
    assert.equal(written, ['- [Docs](README.md)', '- [Architecture Decision Records](adr/README.md)',
        '    - [ADR-NNNN: Area: choice in a few plain words](adr/template.md)', '    - [Decision record tags](adr/tags.md)',
        '    - [ADR-0001: Editor: c](adr/editor/0001-editor-c.md)', '- [Plans](plans/README.md)'].join('\n'));
    assert.equal(spliceSummary('- [Docs](README.md)', []), null);
});
