/**
 * Tests for scripts/prunecache.ts, which deletes the cache entries a save has replaced.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { isListing, replaced } from '../prunecache.ts';

const PREFIX = 'vcpkg-Windows-14.52.37025-';
const SAVED = `${PREFIX}4bdf-300`;

test('prunecache replaces every older entry under the prefix, of any hash', () => {
    const entries = [
        { id: 1, key: `${PREFIX}4bdf-100-ci` },
        { id: 2, key: `${PREFIX}a82b-200-ci-release` },
        { id: 3, key: `${SAVED}-ci` },
    ];
    assert.deepEqual(
        replaced(entries, PREFIX, SAVED).map(({ id }) => id),
        [1, 2],
    );
});

test('prunecache keeps every entry the run saved, so two jobs of one run keep each other', () => {
    const entries = [
        { id: 1, key: SAVED },
        { id: 2, key: `${SAVED}-ci` },
        { id: 3, key: `${SAVED}-ci-release` },
    ];
    assert.deepEqual(replaced(entries, PREFIX, SAVED), []);
});

test('prunecache leaves another toolset and another kind of entry alone', () => {
    const entries = [
        { id: 1, key: 'vcpkg-Windows-14.51.36231-4bdf-100-ci' },
        { id: 2, key: 'mesa-Windows-26.2.0' },
        { id: 3, key: `${SAVED}-ci` },
    ];
    assert.deepEqual(replaced(entries, PREFIX, SAVED), []);
});

test("prunecache does not take a run whose id starts with the saving run's for the same run", () => {
    const entries = [
        { id: 1, key: `${SAVED}1-ci` },
        { id: 2, key: `${SAVED}-ci` },
    ];
    assert.deepEqual(
        replaced(entries, PREFIX, SAVED).map(({ id }) => id),
        [1],
    );
});

test("prunecache deletes nothing when the run's own save is not in the cache", () => {
    const entries = [
        { id: 1, key: `${PREFIX}4bdf-100-ci` },
        { id: 2, key: `${PREFIX}4bdf-200-ci` },
    ];
    assert.deepEqual(replaced(entries, PREFIX, SAVED), []);
});

test('prunecache reads a listing of entries, and refuses anything else', () => {
    assert.equal(isListing({ total_count: 1, actions_caches: [{ id: 1, key: 'k', ref: 'refs/heads/main' }] }), true);
    assert.equal(isListing({ actions_caches: [] }), true);
    assert.equal(isListing({ actions_caches: [{ id: '1', key: 'k' }] }), false);
    assert.equal(isListing({ message: 'Not Found' }), false);
    assert.equal(isListing(null), false);
});
