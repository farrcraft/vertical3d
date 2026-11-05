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
import { type Capture, captureArguments, PICTURES, withShaders } from '../pictures.ts';

test('pictures makes each picture from files in the tree, into docs/', () => {
    for (const picture of PICTURES) {
        assert.ok(picture.image.startsWith('docs/') && picture.image.endsWith('.png'), picture.image);
        const sources = picture.kind === 'render' ? [picture.scene, picture.shaders] : [picture.script];
        for (const source of sources) {
            if (source !== undefined) {
                assert.ok(existsSync(path.join(ROOT, source)), source);
            }
        }
    }
});

test('pictures starts an app with its script, then the capture and its frame', () => {
    const picture: Capture = { kind: 'capture', image: 'docs/images/x.png', app: 'demos/x/x', after: 30, script: 'docs/x.ts' };
    assert.deepEqual(captureArguments(picture),
        ['--script', path.join(ROOT, 'docs/x.ts'), '--capture', path.join(ROOT, 'docs/images/x.png'), '--capture-after', '30']);
    const { script: _script, ...unscripted } = picture;
    assert.deepEqual(captureArguments(unscripted).slice(0, 1), ['--capture']);
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
