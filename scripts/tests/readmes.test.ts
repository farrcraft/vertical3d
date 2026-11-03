/**
 * Tests for scripts/readmes.ts, the check that each component of the tree has a sound README.
 *
 * Each test gives the check a list of files, or one README with the paths that exist beside it.
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { formatFinding } from '../lib/cli.ts';
import { components, firstParagraph, problems, relativeLinks } from '../readmes.ts';

const LIBRARIES = [{ name: 'grid', directory: 'grid' }, { name: 'asset_media', directory: 'asset/media' }];

/** The problems with one README, as the lines the gate prints. Only the listed paths exist. */
function check(directory: string, text: string | null, description?: string, existing: string[] = []): string[] {
    const paths = new Set(existing);
    return problems(directory, text, description, (file) => paths.has(file)).map(formatFinding);
}

test('readmes finds the root directories, the demos and examples with a CMakeLists, and the libraries', () => {
    const files = [
        'README.md',
        '.github/workflows/ctest.yml',
        'moya/moya.cxx',
        'demos/CMakeLists.txt',
        'demos/pong/CMakeLists.txt',
        'demos/pong/src/Game.cxx',
        'examples/starter/CMakeLists.txt',
        'examples/notes/plan.txt',
    ];
    assert.deepEqual(components(files, LIBRARIES),
        ['api/asset/media', 'api/grid', 'demos', 'demos/pong', 'examples', 'examples/starter', 'moya']);
});

test('readmes reports a component with no README', () => {
    assert.deepEqual(check('moya', null), ['moya/README.md: missing: moya has no README.md']);
});

test('readmes accepts a README with a heading and links that resolve', () => {
    const text = '# moya\n\nThe renderer. [docs](../docs/offline/README.md#running) and [libmoya](libmoya/).\n';
    assert.deepEqual(check('moya', text, undefined, ['docs/offline/README.md', 'moya/libmoya']), []);
});

test('readmes reports a README whose first line is not a heading', () => {
    assert.deepEqual(check('moya', 'The renderer.\n'), ['moya/README.md:1: no-heading: the first line is not a "# " heading']);
});

test('readmes reports a relative link to a path that does not exist', () => {
    assert.deepEqual(check('moya', '# moya\n\nSee [the docs](../docs/Moya.md).\n'),
        ['moya/README.md: broken-link: ../docs/Moya.md does not exist']);
});

test('readmes reads only relative links, without their anchors', () => {
    const text = '[a](https://example.com) [b](#here) [c](mailto:x@y.z) [d](../d.md#part) [e](e/)';
    assert.deepEqual(relativeLinks(text), ['../d.md', 'e/']);
});

test('readmes reads the first paragraph after the heading as one line of text', () => {
    assert.deepEqual(firstParagraph('# grid\n\nTile boards: paths and\n[sight](x.md).\n\nMore.\n'),
        'Tile boards: paths and sight.');
});

test('readmes accepts a library README whose first paragraph is its description', () => {
    const text = '# grid\n\nTile boards and\nline of sight.\n\nIts target is `v3dlib_grid`.\n';
    assert.deepEqual(check('api/grid', text, 'Tile boards and line of sight.'), []);
});

test('readmes compares a description with each link read as its text', () => {
    const text = '# curve\n\nCurves. See [Curves.md](../../docs/api/Curves.md).\n';
    assert.deepEqual(check('api/curve', text, 'Curves. See [Curves.md](Curves.md).', ['docs/api/Curves.md']), []);
});

test('readmes reports a library README whose first paragraph differs from its description', () => {
    const text = '# grid\n\nTile boards.\n';
    assert.deepEqual(check('api/grid', text, 'Tile boards and line of sight.'),
        ['api/grid/README.md: stale-description: the first paragraph is not the description in docs/api/README.md: '
            + '"Tile boards and line of sight."']);
});
