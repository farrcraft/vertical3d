// Generate three_primitives.glb, the fixture the gltf loader is asserted against.
//
// One mesh with three primitives, each a separate triangle stepped along x, so a merge that loses
// one shows in the positions rather than only in a count:
//
//   primitive 0  indexed, at x = 0.
//   primitive 1  indexed, at x = 2.     Its indices run 0,1,2 like the first one's, so the
//                                       merge is correct only if they are rebased.
//   primitive 2  not indexed, at x = 4. Merging it needs an index run made for it, or its
//                                       geometry is dropped rather than drawn.
//
// The merged model is therefore nine vertices and the index run 0..8.
//
// The material carries a base colour factor and names an external base colour texture. The image
// is never written. A loader hands the name over for the asset manager to resolve, so nothing
// here decodes one, and a fixture that shipped an image would be testing the png reader instead.
//
// Run from the root of the tree:
//   node api/asset/tests/data/make_model_fixture.ts

import {
    ARRAY_BUFFER, ASSET, ELEMENT_ARRAY_BUFFER, FLOAT, UNSIGNED_SHORT, Writer, bounds, f32, float, floats, u16,
} from './gltf_writer.ts';

const TRIANGLES = [
    [[0, 0, 0], [1, 0, 0], [0, 1, 0]],
    [[2, 0, 0], [3, 0, 0], [2, 1, 0]],
    [[4, 0, 0], [5, 0, 0], [4, 1, 0]],
];
const INDEXED = [true, true, false];
const NORMAL = [0, 0, 1];
const UVS = [[0, 0], [1, 0], [0, 1]];

const writer = new Writer();

const primitives = TRIANGLES.map((triangle, at) => {
    const primitive: Record<string, unknown> = {
        attributes: {
            POSITION: writer.accessor(f32(...triangle.flat()), FLOAT, 3, 'VEC3', ARRAY_BUFFER, bounds(triangle)),
            NORMAL: writer.accessor(f32(...triangle.flatMap(() => NORMAL)), FLOAT, 3, 'VEC3', ARRAY_BUFFER),
            TEXCOORD_0: writer.accessor(f32(...UVS.flat()), FLOAT, 3, 'VEC2', ARRAY_BUFFER),
        },
        material: 0,
    };
    if (INDEXED[at]) {
        primitive.indices = writer.accessor(u16(0, 1, 2), UNSIGNED_SHORT, 3, 'SCALAR', ELEMENT_ARRAY_BUFFER);
    }
    return primitive;
});

writer.glb({
    asset: ASSET,
    scene: 0,
    scenes: [{ nodes: [0] }],
    nodes: [{ mesh: 0 }],
    meshes: [{ primitives }],
    materials: [{
        pbrMetallicRoughness: {
            baseColorFactor: floats([0.25, 0.5, 0.75, 1]),
            baseColorTexture: { index: 0 },
            metallicFactor: float(0),
            roughnessFactor: float(1),
        },
    }],
    textures: [{ source: 0 }],
    images: [{ uri: 'albedo.png' }],
}, 'three_primitives.glb');
