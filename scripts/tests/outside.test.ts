/**
 * Tests for scripts/docs/outside.ts, the preprocessor that points each link the site cannot
 * serve at the file on GitHub.
 *
 * Each test gives the rewrite a few pages and paths that exist. Run them with
 * node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { type Broken, rewrite, type Site, siteTarget } from '../docs/outside.ts';

const REPO = 'https://github.com/owner/repo';

/** A site of the given pages, where only the listed paths exist. */
function site(pages: string[], files: string[], directories: string[] = []): Site {
    return {
        pages: new Set(pages),
        repository: REPO,
        branch: 'main',
        kind: (file) => (files.includes(file) ? 'file' : directories.includes(file) ? 'directory' : null),
    };
}

const SITE = site(['README.md', 'api/README.md', 'api/Grid.md'],
    ['docs/README.md', 'docs/api/README.md', 'docs/api/Grid.md', 'docs/plans/completed/Old.md',
        'docs/images/editor.png', 'api/grid/Grid.h', 'CONTRIBUTING.md'],
    ['docs/api', 'api/grid', 'docs/plans/completed']);

test('outside keeps a link to a page of the site, with its anchor', () => {
    assert.equal(siteTarget('api/README.md', 'Grid.md#paths', SITE), 'Grid.md#paths');
});

test('outside keeps a link to a file in docs/ that is not Markdown', () => {
    assert.equal(siteTarget('api/Grid.md', '../images/editor.png', SITE), '../images/editor.png');
});

test('outside points a link to a directory at its README page', () => {
    assert.equal(siteTarget('README.md', 'api/', SITE), 'api/README.md');
    assert.equal(siteTarget('README.md', 'api', SITE), 'api/README.md');
});

test('outside points a link to a file outside docs/ at the file on GitHub', () => {
    assert.equal(siteTarget('api/Grid.md', '../../api/grid/Grid.h#L10', SITE), `${REPO}/blob/main/api/grid/Grid.h#L10`);
    assert.equal(siteTarget('README.md', '../CONTRIBUTING.md', SITE), `${REPO}/blob/main/CONTRIBUTING.md`);
});

test('outside points a link to a directory outside docs/ at the directory on GitHub', () => {
    assert.equal(siteTarget('api/Grid.md', '../../api/grid/', SITE), `${REPO}/tree/main/api/grid`);
});

test('outside points a link to a completed record at the record on GitHub', () => {
    assert.equal(siteTarget('README.md', 'plans/completed/Old.md', SITE), `${REPO}/blob/main/docs/plans/completed/Old.md`);
    assert.equal(siteTarget('README.md', 'plans/completed/', SITE), `${REPO}/tree/main/docs/plans/completed`);
});

test('outside leaves a web link, an anchor and a mail link as they are', () => {
    for (const target of ['https://example.com/a.md', '#section', 'mailto:a@b.c']) {
        assert.equal(siteTarget('README.md', target, SITE), target);
    }
});

test('outside finds nothing for a relative link to a path that does not exist', () => {
    assert.equal(siteTarget('README.md', '../api/NoSuch.h', SITE), null);
});

test('outside rewrites each link of a page and lists each link that names nothing', () => {
    const broken: Broken[] = [];
    const content = 'See [the header](../../api/grid/Grid.h) and [gone](Gone.md).';
    assert.equal(rewrite('api/Grid.md', content, SITE, broken),
        `See [the header](${REPO}/blob/main/api/grid/Grid.h) and [gone](Gone.md).`);
    assert.deepEqual(broken, [{ page: 'api/Grid.md', target: 'Gone.md' }]);
});

test('outside leaves the links in code spans and fenced blocks as they are', () => {
    const broken: Broken[] = [];
    const content = ['Write `[a](gone.md)` here.', '```', '[b](gone.md)', '```', '~~~~', '[c](gone.md)', '~~~~'].join('\n');
    assert.equal(rewrite('README.md', content, SITE, broken), content);
    assert.deepEqual(broken, []);
});
