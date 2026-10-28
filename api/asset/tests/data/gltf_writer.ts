// Writes glTF test fixtures by hand, for the make_*_fixture.ts generators beside it.
//
// A Writer collects the binary data of one file: each piece is appended to one buffer, aligned
// to 4 bytes, and gets a buffer view and, where asked, an accessor. glb() writes the document
// and the buffer as a binary .glb. gltf() writes a .gltf whose buffer is inlined as a data uri.
//
// A glTF number is written as an integer unless it is wrapped in float(), which writes it with a
// decimal point: 1 as "1.0". The committed fixtures were written that way, and the generators
// reproduce them byte for byte.
//
// Node runs this file directly: it uses only node: modules and type annotations Node can strip.

import { Buffer } from 'node:buffer';
import { writeFileSync } from 'node:fs';
import { join } from 'node:path';

export const ARRAY_BUFFER = 34962;
export const ELEMENT_ARRAY_BUFFER = 34963;

export const UNSIGNED_BYTE = 5121;
export const UNSIGNED_SHORT = 5123;
export const FLOAT = 5126;

const GLB_MAGIC = 0x46546C67;
const JSON_CHUNK = 0x4E4F534A;
const BIN_CHUNK = 0x004E4942;

/** The generator name every fixture's asset block carries. */
export const ASSET = { version: '2.0', generator: 'vertical3d test fixture generator' };

/**
 * A number written with a decimal point when it is a whole number, so 1 is written as "1.0".
 * Outside the range below, Python and JavaScript write a number differently, so it is refused.
 */
export function float(value: number): unknown {
    const size = Math.abs(value);
    if (!Number.isFinite(value) || (size !== 0 && (size < 1e-4 || size >= 1e16))) {
        throw new Error(`float(${value}) is outside the range a fixture writes`);
    }
    let text = Object.is(value, -0) ? '-0' : String(value);
    if (Number.isInteger(value)) {
        text += '.0';
    }
    return (JSON as unknown as { rawJSON(text: string): unknown }).rawJSON(text);
}

/** Every number of a list written as float() writes it. */
export function floats(values: number[]): unknown[] {
    return values.map(float);
}

/** Little endian 32 bit floats. */
export function f32(...values: number[]): Buffer {
    const out = Buffer.alloc(values.length * 4);
    values.forEach((value, at) => out.writeFloatLE(value, at * 4));
    return out;
}

/** Little endian unsigned 16 bit integers. */
export function u16(...values: number[]): Buffer {
    const out = Buffer.alloc(values.length * 2);
    values.forEach((value, at) => out.writeUInt16LE(value, at * 2));
    return out;
}

/** Unsigned bytes. */
export function u8(...values: number[]): Buffer {
    return Buffer.from(values);
}

/** The smallest and largest of each component of a list of vectors, as floats. */
export function bounds(vectors: number[][]): { min: unknown[]; max: unknown[] } {
    const components = vectors[0].map((_, at) => vectors.map((vector) => vector[at]));
    return {
        min: floats(components.map((values) => Math.min(...values))),
        max: floats(components.map((values) => Math.max(...values))),
    };
}

/** The directory the fixtures are written to, which is this file's own. */
export const HERE = import.meta.dirname;

/** Write a file into the fixture directory and return its path. */
function save(name: string, data: Buffer | string): string {
    const path = join(HERE, name);
    writeFileSync(path, data);
    return path;
}

export class Writer {
    blob: Buffer = Buffer.alloc(0);
    views: Record<string, unknown>[] = [];
    accessors: Record<string, unknown>[] = [];

    /**
     * Append data to the buffer, 4 byte aligned, and return its buffer view's index. An image's
     * view carries no target, because it is not vertex or index data.
     */
    view(data: Buffer, target?: number): number {
        const padding = (4 - (this.blob.length % 4)) % 4;
        const offset = this.blob.length + padding;
        this.blob = Buffer.concat([this.blob, Buffer.alloc(padding), data]);
        const view: Record<string, unknown> = { buffer: 0, byteOffset: offset, byteLength: data.length };
        if (target !== undefined) {
            view.target = target;
        }
        this.views.push(view);
        return this.views.length - 1;
    }

    /**
     * Append data with an accessor over it, and return the accessor's index. The properties in
     * extra follow the four every accessor has, in the order given.
     */
    accessor(data: Buffer, component: number, count: number, type: string, target?: number,
        extra: Record<string, unknown> = {}): number {
        this.accessors.push({ bufferView: this.view(data, target), componentType: component, count, type, ...extra });
        return this.accessors.length - 1;
    }

    /**
     * Write the document as a .glb, with the accessors, buffer views and buffer appended to it.
     * The JSON chunk is padded with spaces and the binary chunk with zeros, as glTF requires.
     */
    glb(document: Record<string, unknown>, name: string): void {
        const json = Buffer.from(JSON.stringify({ ...document, ...this.tables() }), 'utf8');
        const jsonChunk = Buffer.concat([json, Buffer.alloc((4 - (json.length % 4)) % 4, ' ')]);
        const binChunk = Buffer.concat([this.blob, Buffer.alloc((4 - (this.blob.length % 4)) % 4)]);

        const header = Buffer.alloc(12);
        header.writeUInt32LE(GLB_MAGIC, 0);
        header.writeUInt32LE(2, 4);
        header.writeUInt32LE(12 + 8 + jsonChunk.length + 8 + binChunk.length, 8);
        const out = Buffer.concat([header, chunk(jsonChunk, JSON_CHUNK), chunk(binChunk, BIN_CHUNK)]);
        console.log(`wrote ${save(name, out)} (${out.length} bytes)`);
    }

    /**
     * Write the document as a .gltf, indented by one space, with the buffer inlined as a data uri
     * after the accessors and buffer views.
     */
    gltf(document: Record<string, unknown>, name: string): void {
        const uri = `data:application/octet-stream;base64,${this.blob.toString('base64')}`;
        const tables = { ...this.tables(), buffers: [{ byteLength: this.blob.length, uri }] };
        console.log(`wrote ${save(name, JSON.stringify({ ...document, ...tables }, null, 1) + '\n')}`);
    }

    /**
     * Write the document as a .gltf, indented by one space, with the buffer in a file of its own
     * beside it. The document names the file by uri, which is the file name percent-encoded.
     */
    gltfBeside(document: Record<string, unknown>, name: string, buffer: string, uri: string): void {
        console.log(`wrote ${save(buffer, this.blob)} (${this.blob.length} bytes)`);
        const tables = { ...this.tables(), buffers: [{ byteLength: this.blob.length, uri }] };
        console.log(`wrote ${save(name, JSON.stringify({ ...document, ...tables }, null, 1) + '\n')}`);
    }

    /** The accessors, buffer views and buffer, in the order a document lists them. */
    private tables(): Record<string, unknown> {
        return { accessors: this.accessors, bufferViews: this.views, buffers: [{ byteLength: this.blob.length }] };
    }
}

/** A .glb chunk: its length, its type and its data. */
function chunk(data: Buffer, type: number): Buffer {
    const header = Buffer.alloc(8);
    header.writeUInt32LE(data.length, 0);
    header.writeUInt32LE(type, 4);
    return Buffer.concat([header, data]);
}
