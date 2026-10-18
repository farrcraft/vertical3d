// Generate primitive_modes.gltf and data_uri_texture.gltf, the fixtures for the parts of the gltf
// loader that no exporter writes.
//
// primitive_modes.gltf is one mesh with four primitives over the same four corners of a unit
// square, each drawn a different way:
//
//   primitive 0  a triangle strip of the four corners, which is two triangles.
//   primitive 1  a triangle fan of the four corners, which is two triangles.
//   primitive 2  lines, which a model of triangles leaves out.
//   primitive 3  points, which it leaves out too.
//
// The merged model is therefore twelve indices over the strip's and the fan's eight vertices. The
// corners are ordered so that both the strip and the fan wind every triangle counter clockwise,
// seen from +z, when they are read as glTF reads them.
//
// Its material names a texture whose file name has a space in it, which a uri carries
// percent-encoded as "my%20texture.png".
//
// data_uri_texture.gltf is one triangle whose base colour texture is pixel.png inlined as a data
// uri, with no mimeType property. The property is optional for a data uri, which states the type
// itself.
//
// Both are .gltf rather than .glb, and carry their buffer as a data uri, so each is one file.
//
// Run from the root of the tree:
//   node api/asset/tests/data/make_modes_fixture.ts

import { readFileSync } from 'node:fs';
import { join } from 'node:path';

import { ARRAY_BUFFER, ASSET, FLOAT, HERE, Writer, bounds, f32 } from './gltf_writer.ts';

const POINTS = 0;
const LINES = 1;
const TRIANGLE_STRIP = 5;
const TRIANGLE_FAN = 6;

// bottom left, bottom right, top left, top right
const STRIP = [[0, 0, 0], [1, 0, 0], [0, 1, 0], [1, 1, 0]];
// round the square from the bottom left, counter clockwise
const FAN = [[0, 0, 0], [1, 0, 0], [1, 1, 0], [0, 1, 0]];

/** Append corners as a position accessor and return its index. */
function positions(writer: Writer, corners: number[][]): number {
    return writer.accessor(f32(...corners.flat()), FLOAT, corners.length, 'VEC3', ARRAY_BUFFER, bounds(corners));
}

function modes(): void {
    const writer = new Writer();
    const drawn: [number[][], number][] = [[STRIP, TRIANGLE_STRIP], [FAN, TRIANGLE_FAN], [STRIP, LINES], [STRIP, POINTS]];
    const primitives = drawn.map(([corners, mode]) => ({ attributes: { POSITION: positions(writer, corners) }, mode, material: 0 }));
    writer.gltf({
        asset: ASSET,
        scene: 0,
        scenes: [{ nodes: [0] }],
        nodes: [{ mesh: 0 }],
        meshes: [{ primitives }],
        materials: [{ pbrMetallicRoughness: { baseColorTexture: { index: 0 } } }],
        textures: [{ source: 0 }],
        images: [{ uri: 'my%20texture.png' }],
    }, 'primitive_modes.gltf');
}

function dataUri(): void {
    const writer = new Writer();
    const position = positions(writer, [[0, 0, 0], [1, 0, 0], [0, 1, 0]]);
    const pixel = readFileSync(join(HERE, 'pixel.png'));
    writer.gltf({
        asset: ASSET,
        scene: 0,
        scenes: [{ nodes: [0] }],
        nodes: [{ mesh: 0 }],
        meshes: [{ primitives: [{ attributes: { POSITION: position }, material: 0 }] }],
        materials: [{ pbrMetallicRoughness: { baseColorTexture: { index: 0 } } }],
        textures: [{ source: 0 }],
        // no mimeType, because the uri states the type
        images: [{ uri: `data:image/png;base64,${pixel.toString('base64')}` }],
    }, 'data_uri_texture.gltf');
}

modes();
dataUri();
