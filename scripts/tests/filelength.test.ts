/**
 * Tests for scripts/filelength.ts, the check that no C++ file but a test is too long.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { LIMIT, checked, lineCount } from '../filelength.ts';

test('filelength checks C++ sources and headers, but not tests or vendored files', () => {
    assert.equal(checked('vertical3d/src/Controller.cxx'), true);
    assert.equal(checked('api/ui/Immediate.h'), true);
    assert.equal(checked('api/ui/paint/ComponentRenderer.cpp'), true);
    assert.equal(checked('api/render/tests/device/LitSceneTest.cpp'), false);
    assert.equal(checked('vertical3d/tests/store/SceneStoreTest.cxx'), false);
    assert.equal(checked('vendor/libnoise/src/noise.cpp'), false);
    assert.equal(checked('demos/voxel/src/noise/noiseutils.cpp'), false);
    assert.equal(checked('scripts/filelength.ts'), false);
});

test('filelength counts a line without a final line break, and not an empty one after one', () => {
    assert.equal(lineCount(''), 0);
    assert.equal(lineCount('one'), 1);
    assert.equal(lineCount('one\n'), 1);
    assert.equal(lineCount('one\ntwo'), 2);
    assert.equal(lineCount('one\n\n'), 2);
});

test('filelength allows a file of exactly the limit', () => {
    const text = 'x\n'.repeat(LIMIT);
    assert.equal(lineCount(text) > LIMIT, false);
    assert.equal(lineCount(text + 'x\n') > LIMIT, true);
});
