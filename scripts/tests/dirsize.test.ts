/**
 * Tests for scripts/dirsize.ts, the check that no directory of C++ code holds too many headers.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { check, counted, headerCounts } from '../dirsize.ts';

test('dirsize counts headers, but not sources, tests or vendored files', () => {
    assert.equal(counted('api/font/face/Face.h'), true);
    assert.equal(counted('vertical3d/src/scene/Scene.h'), true);
    assert.equal(counted('api/font/face/Face.cxx'), false);
    assert.equal(counted('api/ui/paint/TextRenderer.cpp'), false);
    assert.equal(counted('api/render/tests/device/Fixture.h'), false);
    assert.equal(counted('vendor/x/src/noise.h'), false);
});

test('dirsize counts the headers directly in a directory, not in its subdirectories', () => {
    const counts = headerCounts(['api/font/Font.h', 'api/font/face/Face.h', 'api/font/face/Family.h', 'api/font/face/Face.cxx', 'Root.h']);
    assert.equal(counts.get('api/font'), 1);
    assert.equal(counts.get('api/font/face'), 2);
    assert.equal(counts.get('.'), 1);
});

test('dirsize allows a directory of exactly the limit, and reports one over it', () => {
    assert.deepEqual(check(new Map([['api/a', 3]]), 3, new Map()), []);
    const findings = check(new Map([['api/a', 4]]), 3, new Map());
    assert.equal(findings.length, 1);
    assert.equal(findings[0]?.rule, 'over-limit');
    assert.equal(findings[0]?.directory, 'api/a');
});

test('dirsize allows a listed directory over the limit', () => {
    assert.deepEqual(check(new Map([['api/a', 9]]), 3, new Map([['api/a', 'one family']])), []);
});

test('dirsize reports a listed directory within the limit, or gone, as stale', () => {
    const findings = check(new Map([['api/a', 3]]), 3, new Map([['api/a', 'one family'], ['api/gone', 'one family']]));
    assert.deepEqual(findings.map((f) => [f.directory, f.rule]), [['api/a', 'stale-listing'], ['api/gone', 'stale-listing']]);
});
