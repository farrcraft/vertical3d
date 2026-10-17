// Generate two_surfaces.glb and two_surfaces_no_scene.glb, the fixtures a model's parts and its
// node placement are asserted against.
//
// Two meshes under one parent node:
//
//   node 0  the parent, a quarter turn about +z given as a matrix, so (x, y, z) -> (-y, x, z)
//           exactly. A quaternion would put sin(45 degrees) in every term.
//   node 1  translated (2, 0, 0), naming mesh 0.
//   node 2  translated (0, 3, 0), naming mesh 1.
//
//   mesh 0  primitive A, material 0, indexed, at z = 0.
//   mesh 1  primitive B, material 1, indexed, at z = 1.
//           primitive C, material 0, not indexed, at z = 2.
//
// Each primitive is the triangle (0,0), (1,0), (0,1) at its own z, and z survives the turn, so a
// vertex's z says which primitive it came from. Every normal is +x, which is not the face's, so
// that the turn shows in the normals as well: it reads back as +y.
//
// Material 0 is reached first, so it is part 0 and holds A and then C, in the order the walk
// reaches them. Material 1 is part 1 and holds B. Nine vertices, nine indices, two parts.
//
// The second file is the first with no scene, which a loader reads by walking every root.
//
// Run from the root of the tree:
//   node api/asset/tests/data/make_parts_fixture.ts

import {
    ARRAY_BUFFER, ASSET, ELEMENT_ARRAY_BUFFER, FLOAT, UNSIGNED_SHORT, Writer, bounds, f32, float, floats, u16,
} from './gltf_writer.ts';

const TRIANGLE = [[0, 0], [1, 0], [0, 1]];
const NORMAL = [1, 0, 0];
const QUARTER_TURN_ABOUT_Z = [0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1];

const writer = new Writer();

/** One triangle at the given z, and the primitive drawing it. */
function primitive(z: number, material: number, indexed: boolean): Record<string, unknown> {
    const triangle = TRIANGLE.map(([x, y]) => [x, y, z]);
    const result: Record<string, unknown> = {
        attributes: {
            POSITION: writer.accessor(f32(...triangle.flat()), FLOAT, 3, 'VEC3', ARRAY_BUFFER, bounds(triangle)),
            NORMAL: writer.accessor(f32(...triangle.flatMap(() => NORMAL)), FLOAT, 3, 'VEC3', ARRAY_BUFFER),
        },
        material,
    };
    if (indexed) {
        result.indices = writer.accessor(u16(0, 1, 2), UNSIGNED_SHORT, 3, 'SCALAR', ELEMENT_ARRAY_BUFFER);
    }
    return result;
}

/** A material of one colour, naming a base colour texture when given one. */
function material(colour: number[], texture?: number): Record<string, unknown> {
    const pbr: Record<string, unknown> = { baseColorFactor: floats(colour), metallicFactor: float(0), roughnessFactor: float(1) };
    if (texture !== undefined) {
        pbr.baseColorTexture = { index: texture };
    }
    return { pbrMetallicRoughness: pbr };
}

const meshes = [
    { primitives: [primitive(0, 0, true)] },
    { primitives: [primitive(1, 1, true), primitive(2, 0, false)] },
];

const document: Record<string, unknown> = {
    asset: ASSET,
    scene: 0,
    scenes: [{ nodes: [0] }],
    nodes: [
        { matrix: QUARTER_TURN_ABOUT_Z, children: [1, 2] },
        { translation: floats([2, 0, 0]), mesh: 0 },
        { translation: floats([0, 3, 0]), mesh: 1 },
    ],
    meshes,
    materials: [material([1, 0, 0, 1], 0), material([0, 0, 1, 1])],
    textures: [{ source: 0 }],
    images: [{ uri: 'red.png' }],
};

writer.glb(document, 'two_surfaces.glb');

const sceneless = { ...document };
delete sceneless.scene;
delete sceneless.scenes;
writer.glb(sceneless, 'two_surfaces_no_scene.glb');
