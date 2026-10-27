/**
 * Tests for scripts/librarytable.ts, the check that the table of libraries in docs/api/README.md
 * matches the manifest.
 *
 * Each test gives the check a small manifest and a small table. Run them with
 * node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { manifestLibraries, problems, tableRows } from '../librarytable.ts';

const MANIFEST = [
    '# Every api library',
    'set(V3D_API_LIBRARIES',
    '\tasset asset_media log)',
    '',
    'set(V3D_API_asset_PATH "asset")',
    '# under api/asset but a library of its own',
    'set(V3D_API_asset_media_PATH "asset/media")',
    'set(V3D_API_log_PATH "log")',
].join('\n');

/** A README whose library table has the given rows, between other sections. */
function readme(rows: string[]): string {
    return [
        '# The api',
        '',
        '| Not | This table |',
        '| [`log`](../../api/log/) | before the heading |',
        '',
        '## The libraries',
        '',
        '| Library | What it is for |',
        '|---|---|',
        ...rows,
        '',
        '## How they depend on each other',
        '',
        '| [`log`](../../api/log/) | after the next heading |',
    ].join('\n');
}

const ASSET = '| [`asset`](../../api/asset/) | Files. |';
const MEDIA = '| [`asset_media`](../../api/asset/media/) | Pictures. |';
const LOG = '| [`log`](../../api/log/) | The log. |';

function check(rows: string[]): string[] {
    return problems(manifestLibraries(MANIFEST), tableRows(readme(rows)));
}

test('librarytable reads the libraries over several lines, each with its directory', () => {
    assert.deepEqual(manifestLibraries(MANIFEST), [
        { name: 'asset', directory: 'asset' },
        { name: 'asset_media', directory: 'asset/media' },
        { name: 'log', directory: 'log' },
    ]);
});

test('librarytable reads only the rows under its heading', () => {
    assert.deepEqual(tableRows(readme([ASSET, LOG])).map((row) => row.name), ['asset', 'log']);
});

test('librarytable accepts a row for each library in the manifest order', () => {
    assert.deepEqual(check([ASSET, MEDIA, LOG]), []);
});

test('librarytable reports a library with no row, and a row for no library', () => {
    const found = check([ASSET, MEDIA, '| [`audio`](../../api/audio/) | Sound. |']);
    assert.equal(found.length, 2);
    assert.match(found[0], /audio is not a library/);
    assert.match(found[1], /log has no row/);
});

test('librarytable reports a second row for one library', () => {
    const found = check([ASSET, MEDIA, LOG, LOG]);
    assert.equal(found.length, 1);
    assert.match(found[0], /log has a row already/);
});

test('librarytable reports a row that links another directory', () => {
    const found = check([ASSET, '| [`asset_media`](../../api/media/) | Pictures. |', LOG]);
    assert.equal(found.length, 1);
    assert.match(found[0], /asset_media links \.\.\/\.\.\/api\/media\/, and its directory is \.\.\/\.\.\/api\/asset\/media\//);
});

test('librarytable reports rows out of the manifest order', () => {
    const found = check([MEDIA, ASSET, LOG]);
    assert.equal(found.length, 2);
    assert.match(found[0], /asset_media is out of order, and the manifest has asset here/);
});

test('librarytable reports a manifest that names no libraries', () => {
    assert.deepEqual(problems(manifestLibraries('set(V3D_API_log_PATH "log")'), []), [
        'cmake/v3dApiLibraries.cmake names no libraries in V3D_API_LIBRARIES',
    ]);
});
