// Generate the fixtures for the gltf loader's edge cases, which no exporter writes:
//
// external_buffer.gltf and "external buffer.bin"
//
//   One triangle whose buffer is a file beside the document rather than a data uri. The file
//   name has a space in it, which the uri carries percent-encoded as "external%20buffer.bin".
//
// image_without_source.gltf
//
//   One triangle whose base colour texture is an image with a mimeType and neither a uri nor a
//   buffer view, so it has no bytes to decode.
//
// joint_outside_skin.gltf
//
//   One triangle skinned to a skin of one joint. Vertex 0 gives half its weight to joint 5,
//   which the skin does not have, and half to joint 0. Vertices 1 and 2 are wholly joint 0's.
//
// Run from the root of the tree:
//   node api/asset/tests/data/make_edge_fixtures.ts

import { ARRAY_BUFFER, ASSET, FLOAT, UNSIGNED_SHORT, Writer, bounds, f32, u16 } from './gltf_writer.ts';

const CORNERS = [[0, 0, 0], [1, 0, 0], [0, 1, 0]];

/** Append the triangle's corners as a position accessor and return its index. */
function triangle(writer: Writer): number {
    return writer.accessor(f32(...CORNERS.flat()), FLOAT, CORNERS.length, 'VEC3', ARRAY_BUFFER, bounds(CORNERS));
}

function externalBuffer(): void {
    const writer = new Writer();
    const position = triangle(writer);
    writer.gltfBeside({
        asset: ASSET,
        scene: 0,
        scenes: [{ nodes: [0] }],
        nodes: [{ mesh: 0 }],
        meshes: [{ primitives: [{ attributes: { POSITION: position } }] }],
    }, 'external_buffer.gltf', 'external buffer.bin', 'external%20buffer.bin');
}

function imageWithoutSource(): void {
    const writer = new Writer();
    const position = triangle(writer);
    writer.gltf({
        asset: ASSET,
        scene: 0,
        scenes: [{ nodes: [0] }],
        nodes: [{ mesh: 0 }],
        meshes: [{ primitives: [{ attributes: { POSITION: position }, material: 0 }] }],
        materials: [{ pbrMetallicRoughness: { baseColorTexture: { index: 0 } } }],
        textures: [{ source: 0 }],
        images: [{ mimeType: 'image/png' }],
    }, 'image_without_source.gltf');
}

function jointOutsideSkin(): void {
    const writer = new Writer();
    const position = triangle(writer);
    const joints = writer.accessor(u16(5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0), UNSIGNED_SHORT, 3, 'VEC4', ARRAY_BUFFER);
    const weights = writer.accessor(f32(0.5, 0.5, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0), FLOAT, 3, 'VEC4', ARRAY_BUFFER);
    const identity = writer.accessor(f32(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1), FLOAT, 1, 'MAT4');
    writer.gltf({
        asset: ASSET,
        scene: 0,
        scenes: [{ nodes: [0, 1] }],
        nodes: [{ mesh: 0, skin: 0 }, { name: 'joint' }],
        meshes: [{ primitives: [{ attributes: { POSITION: position, JOINTS_0: joints, WEIGHTS_0: weights } }] }],
        skins: [{ joints: [1], inverseBindMatrices: identity }],
    }, 'joint_outside_skin.gltf');
}

externalBuffer();
imageWithoutSource();
jointOutsideSkin();
