// Generate embedded_texture.glb, the fixture the gltf loader's embedded image path is asserted
// against.
//
// One indexed triangle with a material whose base colour texture is an image the file carries
// rather than names: a buffer view holding the bytes of pixel.png, with the mime type glTF
// requires for one. three_primitives.glb cannot cover this case, because the asset manager
// resolves a named texture and the loader never decodes it.
//
// The image is pixel.png rather than one generated here, so that what the loader decodes can be
// compared against what the png reader makes of the same file.
//
// Run from the root of the tree:
//   node api/asset/tests/data/make_embedded_fixture.ts

import { readFileSync } from 'node:fs';
import { join } from 'node:path';

import {
    ARRAY_BUFFER, ASSET, ELEMENT_ARRAY_BUFFER, FLOAT, HERE, UNSIGNED_SHORT, Writer, bounds, f32, float, floats, u16,
} from './gltf_writer.ts';

const TRIANGLE = [[0, 0, 0], [1, 0, 0], [0, 1, 0]];
const NORMAL = [0, 0, 1];
const UVS = [[0, 0], [1, 0], [0, 1]];

const writer = new Writer();

const position = writer.accessor(f32(...TRIANGLE.flat()), FLOAT, 3, 'VEC3', ARRAY_BUFFER, bounds(TRIANGLE));
const normal = writer.accessor(f32(...TRIANGLE.flatMap(() => NORMAL)), FLOAT, 3, 'VEC3', ARRAY_BUFFER);
const uv = writer.accessor(f32(...UVS.flat()), FLOAT, 3, 'VEC2', ARRAY_BUFFER);
const indices = writer.accessor(u16(0, 1, 2), UNSIGNED_SHORT, 3, 'SCALAR', ELEMENT_ARRAY_BUFFER);
const image = writer.view(readFileSync(join(HERE, 'pixel.png')));

writer.glb({
    asset: ASSET,
    scene: 0,
    scenes: [{ nodes: [0] }],
    nodes: [{ mesh: 0 }],
    meshes: [{
        primitives: [{
            attributes: { POSITION: position, NORMAL: normal, TEXCOORD_0: uv },
            indices,
            material: 0,
        }],
    }],
    materials: [{
        pbrMetallicRoughness: {
            baseColorFactor: floats([1, 1, 1, 1]),
            baseColorTexture: { index: 0 },
            metallicFactor: float(0),
            roughnessFactor: float(1),
        },
    }],
    textures: [{ source: 0 }],
    images: [{ bufferView: image, mimeType: 'image/png' }],
}, 'embedded_texture.glb');
