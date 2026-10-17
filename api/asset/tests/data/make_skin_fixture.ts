// Generate bending_strip.glb and bending_strip_shuffled.glb, the fixtures a skin is asserted
// against. No rigged asset exists in the tree. Every number here is exact in float, so that what a
// loader or a sampler makes of them can be compared with no tolerance.
//
// bending_strip.glb
//
//   A strip two units wide and three tall along +y, a row of two vertices every half unit, bound
//   to a chain of three joints standing at y = 0, 1 and 2:
//
//     node 0  "strip"     the skinned mesh, translated (0, 0, 7). A loader ignores the
//                         translation, because a skinned mesh is placed by its joints alone.
//     node 1  "armature"  above the skeleton, at the identity.
//     node 2  "root"      a joint at (0, 0, 0).
//     node 3  "middle"    a joint, (0, 1, 0) from root.
//     node 4  "top"       a joint, (0, 1, 0) from middle.
//     node 5  "tip"       an unskinned triangle, (0, 0.5, 0) from top, which follows top rigidly.
//
//   Every joint's rotation is the identity and its translation a whole number, so each inverse
//   bind matrix is an exact negated translation. A vertex below y = 1 is wholly root's, one
//   between 1 and 2 wholly middle's, one above 2 wholly top's, and the rows at 1 and 2 are half
//   and half. Every weight is exact. Joints are unsigned shorts and weights floats.
//
//   The strip is material 0 and the tip material 1, so the model is two parts.
//
//   Three clips, each a second long:
//
//     bend   middle turns a quarter about +z, linearly, so top swings to (-1, 1, 0). It also
//            moves the tip, which is not a joint, and which a loader drops.
//     step   the same turn, held at the first key until the second.
//     cubic  top rises from y = 1 to 2 as a cubic spline, with an out tangent of +x and an in
//            tangent of -x, which at a half puts it at (0.25, 1.5, 0).
//
// bending_strip_shuffled.glb
//
//   The same skeleton listed child first in the skin: top, root, middle. A loader reorders it and
//   remaps every vertex's joints. The armature stands at (0, 0, 4), which the inverse bind
//   matrices and the strip's positions include, and which a loader keeps as the skeleton's root.
//   Joints are unsigned bytes and weights normalised unsigned bytes. No tip.
//
// Run from the root of the tree:
//   node api/asset/tests/data/make_skin_fixture.ts

import { Buffer } from 'node:buffer';

import {
    ARRAY_BUFFER, ASSET, ELEMENT_ARRAY_BUFFER, FLOAT, UNSIGNED_BYTE, UNSIGNED_SHORT, Writer, f32, floats, u16, u8,
} from './gltf_writer.ts';

const ROWS = [0, 0.5, 1, 1.5, 2, 2.5, 3];
const COLUMNS = [-1, 1];

// where each joint stands at bind, in the skeleton's own order: root, middle, top
const BIND_Y = [0, 1, 2];

/** The joints a row follows, by skeleton index, and how far. */
function influence(y: number): [number, number][] {
    if (y < 1) {
        return [[0, 1]];
    }
    if (y === 1) {
        return [[0, 0.5], [1, 0.5]];
    }
    if (y < 2) {
        return [[1, 1]];
    }
    if (y === 2) {
        return [[1, 0.5], [2, 0.5]];
    }
    return [[2, 1]];
}

/** A column major 4x4 translation, as glTF stores a matrix. */
function translation(x: number, y: number, z: number): number[] {
    return [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1];
}

/** How a vertex's joints and weights are stored. */
interface Storage {
    joints: (values: number[]) => Buffer;
    jointComponent: number;
    weights: (values: number[]) => Buffer;
    weightComponent: number;
    normalized: boolean;
}

const SHORTS_AND_FLOATS: Storage = {
    joints: (values) => u16(...values),
    jointComponent: UNSIGNED_SHORT,
    weights: (values) => f32(...values),
    weightComponent: FLOAT,
    normalized: false,
};

const BYTES: Storage = {
    joints: (values) => u8(...values),
    jointComponent: UNSIGNED_BYTE,
    weights: (values) => {
        // a half and a half are 128 and 127, which still sum to 255. A loader renormalises.
        const scaled = values.map((value) => Math.round(value * 255));
        const first = scaled.indexOf(128);
        const second = scaled.indexOf(128, first + 1);
        if (first >= 0 && second >= 0) {
            scaled[second] = 127;
        }
        return u8(...scaled);
    },
    weightComponent: UNSIGNED_BYTE,
    normalized: true,
};

/** The strip's mesh: its vertices at the given z, and its joints written by skin index. */
function strip(writer: Writer, z: number, skinIndex: number[], storage: Storage): Record<string, unknown> {
    const positions: number[] = [];
    const normals: number[] = [];
    const joints: Buffer[] = [];
    const weights: Buffer[] = [];
    for (const y of ROWS) {
        for (const x of COLUMNS) {
            positions.push(x, y, z);
            normals.push(0, 0, 1);
            const pairs = influence(y);
            while (pairs.length < 4) {
                pairs.push([0, 0]);
            }
            joints.push(storage.joints(pairs.map(([joint]) => skinIndex[joint])));
            weights.push(storage.weights(pairs.map(([, weight]) => weight)));
        }
    }

    const indices: number[] = [];
    for (let row = 0; row < ROWS.length - 1; row++) {
        const a = row * 2;
        const b = a + 1;
        indices.push(a, b, b + 2, a, b + 2, a + 2);
    }

    const count = ROWS.length * COLUMNS.length;
    const attributes = {
        POSITION: writer.accessor(f32(...positions), FLOAT, count, 'VEC3', ARRAY_BUFFER,
            { min: floats([-1, 0, z]), max: floats([1, 3, z]) }),
        NORMAL: writer.accessor(f32(...normals), FLOAT, count, 'VEC3', ARRAY_BUFFER),
        JOINTS_0: writer.accessor(Buffer.concat(joints), storage.jointComponent, count, 'VEC4', ARRAY_BUFFER),
        WEIGHTS_0: writer.accessor(Buffer.concat(weights), storage.weightComponent, count, 'VEC4', ARRAY_BUFFER,
            storage.normalized ? { normalized: true } : {}),
    };
    const index = writer.accessor(u16(...indices), UNSIGNED_SHORT, indices.length, 'SCALAR', ELEMENT_ARRAY_BUFFER);
    return { primitives: [{ attributes, indices: index, material: 0 }] };
}

/** The inverse bind matrices of the skeleton's joints, in the order the skin lists them. */
function inverseBinds(writer: Writer, skinOrder: number[], armatureZ: number): number {
    const matrices = skinOrder.flatMap((joint) => translation(0, -BIND_Y[joint], -armatureZ));
    return writer.accessor(f32(...matrices), FLOAT, skinOrder.length, 'MAT4');
}

const MATERIALS = [
    { pbrMetallicRoughness: { baseColorFactor: floats([1, 1, 1, 1]) } },
    { pbrMetallicRoughness: { baseColorFactor: floats([1, 0, 0, 1]) } },
];

const QUARTER = Math.sqrt(0.5);

/** The three clips, over the plain file's nodes. */
function animations(writer: Writer): Record<string, unknown>[] {
    const times = writer.accessor(f32(0, 1), FLOAT, 2, 'SCALAR', undefined, { min: floats([0]), max: floats([1]) });
    const turn = writer.accessor(f32(0, 0, 0, 1, 0, 0, QUARTER, QUARTER), FLOAT, 2, 'VEC4');
    const lift = writer.accessor(f32(0, 0, 0, 0, 1, 0), FLOAT, 2, 'VEC3');
    // in tangent, value, out tangent, for each of the two keys
    const rise = writer.accessor(f32(0, 0, 0, 0, 1, 0, 1, 0, 0, -1, 0, 0, 0, 2, 0, 0, 0, 0), FLOAT, 6, 'VEC3');
    return [
        {
            name: 'bend',
            samplers: [{ input: times, output: turn, interpolation: 'LINEAR' },
                { input: times, output: lift, interpolation: 'LINEAR' }],
            channels: [{ sampler: 0, target: { node: 3, path: 'rotation' } },
                { sampler: 1, target: { node: 5, path: 'translation' } }],
        },
        {
            name: 'step',
            samplers: [{ input: times, output: turn, interpolation: 'STEP' }],
            channels: [{ sampler: 0, target: { node: 3, path: 'rotation' } }],
        },
        {
            name: 'cubic',
            samplers: [{ input: times, output: rise, interpolation: 'CUBICSPLINE' }],
            channels: [{ sampler: 0, target: { node: 4, path: 'translation' } }],
        },
    ];
}

function plain(): void {
    const writer = new Writer();
    const skinOrder = [0, 1, 2];
    const tip = writer.accessor(f32(0, 0, 0, 0.25, 0, 0, 0, 0.25, 0), FLOAT, 3, 'VEC3', ARRAY_BUFFER,
        { min: floats([0, 0, 0]), max: floats([0.25, 0.25, 0]) });
    // built in the order the document lists them, so the accessors are numbered in that order
    const meshes = [strip(writer, 0, skinOrder, SHORTS_AND_FLOATS), { primitives: [{ attributes: { POSITION: tip }, material: 1 }] }];
    const skins = [{ joints: [2, 3, 4], inverseBindMatrices: inverseBinds(writer, skinOrder, 0) }];
    writer.glb({
        asset: ASSET,
        scene: 0,
        scenes: [{ nodes: [0, 1] }],
        nodes: [
            { name: 'strip', mesh: 0, skin: 0, translation: floats([0, 0, 7]) },
            { name: 'armature', children: [2] },
            { name: 'root', children: [3] },
            { name: 'middle', translation: floats([0, 1, 0]), children: [4] },
            { name: 'top', translation: floats([0, 1, 0]), children: [5] },
            { name: 'tip', mesh: 1, translation: floats([0, 0.5, 0]) },
        ],
        meshes,
        skins,
        animations: animations(writer),
        materials: MATERIALS,
    }, 'bending_strip.glb');
}

function shuffled(): void {
    const writer = new Writer();
    const skinOrder = [2, 0, 1];
    const skinIndex = [0, 1, 2].map((joint) => skinOrder.indexOf(joint));
    const meshes = [strip(writer, 4, skinIndex, BYTES)];
    const skins = [{ joints: [4, 2, 3], inverseBindMatrices: inverseBinds(writer, skinOrder, 4) }];
    writer.glb({
        asset: ASSET,
        scene: 0,
        scenes: [{ nodes: [0, 1] }],
        nodes: [
            { name: 'strip', mesh: 0, skin: 0 },
            { name: 'armature', translation: floats([0, 0, 4]), children: [2] },
            { name: 'root', children: [3] },
            { name: 'middle', translation: floats([0, 1, 0]), children: [4] },
            { name: 'top', translation: floats([0, 1, 0]) },
        ],
        meshes,
        skins,
        materials: MATERIALS.slice(0, 1),
    }, 'bending_strip_shuffled.glb');
}

plain();
shuffled();
