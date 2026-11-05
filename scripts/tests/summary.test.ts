/**
 * Tests for scripts/summary.ts, the check that docs/SUMMARY.md lists each page of the site once.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { formatFinding } from '../lib/cli.ts';
import { entries, pages, problems } from '../summary.ts';

/** The problems with a summary over the given pages, as the lines the gate prints. */
function check(all: string[], summary: string): string[] {
    return problems(all, entries(summary)).map(formatFinding);
}

test('summary reads the Markdown under docs/, but SUMMARY.md and completed records', () => {
    const files = [
        'README.md',
        'docs/README.md',
        'docs/SUMMARY.md',
        'docs/adr/0001-a.md',
        'docs/adr/template.md',
        'docs/plans/completed/Old.md',
        'docs/images/editor.png',
    ];
    assert.deepEqual(pages(files), ['README.md', 'adr/0001-a.md', 'adr/template.md']);
});

test('summary reads each link of the summary, without its anchor', () => {
    assert.deepEqual(entries('# Summary\n\n[Docs](README.md)\n\n- [A](adr/a.md#top)\n'),
        [{ page: 'README.md', line: 3 }]);
});

test('summary accepts a summary that lists each page once', () => {
    assert.deepEqual(check(['README.md', 'adr/a.md'], '[Docs](README.md)\n- [A](adr/a.md)\n'), []);
});

test('summary reports a page the summary leaves out', () => {
    assert.deepEqual(check(['README.md', 'adr/a.md'], '[Docs](README.md)\n'),
        ['docs/adr/a.md: unlisted: docs/SUMMARY.md does not list it']);
});

test('summary reports an entry that names no page', () => {
    assert.deepEqual(check(['README.md'], '[Docs](README.md)\n- [Gone](gone.md)\n'),
        ['docs/SUMMARY.md:2: no-page: gone.md is not a page under docs/']);
});

test('summary reports a page listed twice', () => {
    assert.deepEqual(check(['README.md'], '[Docs](README.md)\n- [Again](README.md)\n'),
        ['docs/SUMMARY.md:2: second-entry: README.md is listed already']);
});
