/**
 * Tests for scripts/pictures.ts, the script that renders the pictures the documents show.
 *
 * The tests check the list of pictures and the search path a scene is given. They render
 * nothing. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { existsSync } from 'node:fs';
import * as path from 'node:path';
import { test } from 'node:test';

import { ROOT } from '../lib/tree.ts';
import { PICTURES, withShaders } from '../pictures.ts';

test('pictures renders each picture from a scene in the tree, into docs/', () => {
    for (const picture of PICTURES) {
        assert.ok(existsSync(path.join(ROOT, picture.scene)), picture.scene);
        assert.ok(picture.image.startsWith('docs/') && picture.image.endsWith('.png'), picture.image);
        if (picture.shaders !== undefined) {
            assert.ok(existsSync(path.join(ROOT, picture.shaders)), picture.shaders);
        }
    }
});

test('pictures writes each image to a path of its own', () => {
    const images = PICTURES.map((picture) => picture.image);
    assert.equal(new Set(images).size, images.length);
});

test('pictures puts the shaders first on the search path, before the scene', () => {
    assert.equal(withShaders('version 3.03\n', path.join('C:', 'repo', 'shaders')).split('\n')[0],
        `Option "searchpath" "shader" ["${path.join('C:', 'repo', 'shaders').split(path.sep).join('/')}:&"]`);
    assert.ok(withShaders('version 3.03\n', 'shaders').endsWith('version 3.03\n'));
});
