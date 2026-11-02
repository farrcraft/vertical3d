/**
 * Write the colour font fixtures: one font for each OpenType colour table.
 *
 * Every font is 1000 units to the em, 800 above the baseline and 200 below. Its outlines are
 * rectangles: the em square, its left half and its right half. Each colour glyph is mapped from
 * the private use area, from U+E000, and draws one of a few plain pictures, so the colour of
 * every pixel a test reads is known.
 *
 * - colr0.ttf: U+E000 is the left half in palette entry 0 and the right half in entry 1. U+E001
 *   is the left half in entry 0 and the right half in the text's colour.
 * - colr1.ttf: U+E000 is the em square under a linear gradient from entry 0 at its left to entry
 *   1 at its right. U+E001 is layered as colr0's U+E001 is. U+E002 is a radial gradient from
 *   entry 0 at the square's centre to entry 1 at 500 units from it. U+E003 is a sweep gradient
 *   from entry 0 to entry 1, counter-clockwise from the right of the centre. U+E004 is the left
 *   half in entry 2, moved right by 500 units.
 * - svg.ttf: U+E000 is red on the left and blue on the right, both halves drawn by one shared
 *   rectangle. U+E001 is the em square under a linear gradient from red to blue.
 * - cbdt.ttf and sbix.ttf: U+E000 is a 32 pixel PNG at 32 pixels to the em, red on the left and
 *   blue on the right.
 *
 * Palette 0 is red, blue and green, and palette 1 is blue, red and green.
 *
 * Usage: node api/font/tests/fonts/colourfonts.ts
 */

import { writeFileSync } from 'node:fs';
import * as path from 'node:path';
import { crc32, deflateSync } from 'node:zlib';

const UNITS = 1000;
const ASCENDER = 800;
const DESCENDER = -200;
const FIRST = 0xE000;
const FOREGROUND = 0xFFFF;
const STRIKE = 32;

/** A big-endian byte writer. */
class Writer {
    private bytes: number[] = [];

    get length(): number {
        return this.bytes.length;
    }

    u8(value: number): this {
        this.bytes.push(value & 0xFF);
        return this;
    }

    u16(value: number): this {
        return this.u8(value >> 8).u8(value);
    }

    u24(value: number): this {
        return this.u8(value >> 16).u16(value);
    }

    u32(value: number): this {
        return this.u16(Math.floor(value / 0x10000)).u16(value);
    }

    /** A 16.16 fixed point number. */
    fixed(value: number): this {
        return this.u32(Math.round(value * 0x10000) >>> 0);
    }

    /** A 2.14 fixed point number. */
    f2dot14(value: number): this {
        return this.u16(Math.round(value * 0x4000));
    }

    tag(value: string): this {
        for (const c of value) {
            this.u8(c.charCodeAt(0));
        }
        return this;
    }

    append(other: Uint8Array): this {
        for (const b of other) {
            this.bytes.push(b);
        }
        return this;
    }

    /** Zeroes up to a multiple of four bytes. */
    pad(): this {
        while (this.bytes.length % 4 !== 0) {
            this.bytes.push(0);
        }
        return this;
    }

    /** Overwrite a 24 bit value already written. */
    put24(at: number, value: number): void {
        this.bytes[at] = (value >> 16) & 0xFF;
        this.bytes[at + 1] = (value >> 8) & 0xFF;
        this.bytes[at + 2] = value & 0xFF;
    }

    /** Overwrite a 32 bit value already written. */
    put32(at: number, value: number): void {
        this.bytes[at] = (value >>> 24) & 0xFF;
        this.bytes[at + 1] = (value >> 16) & 0xFF;
        this.bytes[at + 2] = (value >> 8) & 0xFF;
        this.bytes[at + 3] = value & 0xFF;
    }

    done(): Uint8Array {
        return Uint8Array.from(this.bytes);
    }
}

/** A rectangle, from its lower left to its upper right, in font units. */
interface Box {
    x0: number;
    y0: number;
    x1: number;
    y1: number;
}

const SQUARE: Box = { x0: 0, y0: DESCENDER, x1: UNITS, y1: ASCENDER };
const LEFT: Box = { x0: 0, y0: DESCENDER, x1: UNITS / 2, y1: ASCENDER };
const RIGHT: Box = { x0: UNITS / 2, y0: DESCENDER, x1: UNITS, y1: ASCENDER };

/** A glyph: its name, its outline, and the code point mapped to it, if any. */
interface Glyph {
    name: string;
    box: Box | null;
    code?: number;
}

/** The glyphs every font starts with: the missing glyph, then the three outlines. */
function outlines(): Glyph[] {
    return [
        { name: '.notdef', box: null },
        { name: 'square', box: SQUARE },
        { name: 'left', box: LEFT },
        { name: 'right', box: RIGHT },
    ];
}

/** A glyph's outline, a rectangle drawn clockwise, as the glyf table holds it. */
function glyf(box: Box | null): Uint8Array {
    if (box === null) {
        return new Uint8Array(0);
    }
    const points = [[box.x0, box.y0], [box.x0, box.y1], [box.x1, box.y1], [box.x1, box.y0]];
    const w = new Writer().u16(1).u16(box.x0).u16(box.y0).u16(box.x1).u16(box.y1).u16(points.length - 1).u16(0);
    for (const _ of points) {
        w.u8(0x01);  // on the curve, with whole 16 bit coordinates
    }
    let previous = [0, 0];
    for (const axis of [0, 1]) {
        previous = [0, 0];
        for (const point of points) {
            w.u16(point[axis] - previous[axis]);
            previous = point;
        }
    }
    return w.pad().done();
}

function head(): Uint8Array {
    return new Writer().fixed(1).fixed(1).u32(0).u32(0x5F0F3CF5).u16(0x000B).u16(UNITS)
        .u32(0).u32(0).u32(0).u32(0)
        .u16(SQUARE.x0).u16(SQUARE.y0).u16(SQUARE.x1).u16(SQUARE.y1)
        .u16(0).u16(8).u16(2).u16(1).u16(0).done();
}

function hhea(count: number): Uint8Array {
    return new Writer().fixed(1).u16(ASCENDER).u16(DESCENDER).u16(0).u16(UNITS).u16(0).u16(0).u16(UNITS)
        .u16(1).u16(0).u16(0).u16(0).u16(0).u16(0).u16(0).u16(0).u16(count).done();
}

function maxp(count: number): Uint8Array {
    const w = new Writer().fixed(1).u16(count).u16(4).u16(1).u16(0).u16(0).u16(2);
    for (let i = 0; i < 8; i++) {
        w.u16(0);
    }
    return w.done();
}

function os2(glyphs: Glyph[]): Uint8Array {
    const codes = glyphs.flatMap((g) => (g.code === undefined ? [] : [g.code]));
    const w = new Writer().u16(4).u16(UNITS).u16(400).u16(5).u16(0);
    for (const value of [650, 600, 0, 75, 650, 600, 0, 350, 50, 300]) {
        w.u16(value);
    }
    w.u16(0);
    for (let i = 0; i < 10; i++) {
        w.u8(0);
    }
    w.u32(0).u32(0).u32(0).u32(0).tag('V3D ');
    // regular, and measured by its typographic metrics
    w.u16(0x40 | 0x80).u16(Math.min(...codes)).u16(Math.max(...codes));
    w.u16(ASCENDER).u16(DESCENDER).u16(0).u16(ASCENDER).u16(-DESCENDER).u32(1).u32(0);
    return w.u16(500).u16(700).u16(0).u16(32).u16(1).done();
}

function hmtx(glyphs: Glyph[]): Uint8Array {
    const w = new Writer();
    for (const glyph of glyphs) {
        w.u16(UNITS).u16(glyph.box === null ? 0 : glyph.box.x0);
    }
    return w.done();
}

/** A format 4 character map with one segment for each code point. */
function cmap(glyphs: Glyph[]): Uint8Array {
    const mapped = glyphs.flatMap((g, index) => (g.code === undefined ? [] : [{ code: g.code, index }]));
    mapped.sort((a, b) => a.code - b.code);
    const segments = [...mapped.map((m) => ({ start: m.code, end: m.code, delta: (m.index - m.code) & 0xFFFF })),
        { start: 0xFFFF, end: 0xFFFF, delta: 1 }];
    const count = segments.length;
    const power = 2 ** Math.floor(Math.log2(count));
    const sub = new Writer().u16(4).u16(16 + (count * 8)).u16(0).u16(count * 2)
        .u16(power * 2).u16(Math.log2(power)).u16((count - power) * 2);
    segments.forEach((s) => sub.u16(s.end));
    sub.u16(0);
    segments.forEach((s) => sub.u16(s.start));
    segments.forEach((s) => sub.u16(s.delta));
    segments.forEach(() => sub.u16(0));
    return new Writer().u16(0).u16(1).u16(3).u16(1).u32(12).append(sub.done()).done();
}

function name(family: string): Uint8Array {
    const strings = [[1, family], [2, 'Regular'], [4, `${family} Regular`], [6, `${family.replaceAll(' ', '')}-Regular`]] as const;
    const text = new Writer();
    const w = new Writer().u16(0).u16(strings.length).u16(6 + (strings.length * 12));
    for (const [id, value] of strings) {
        const start = text.length;
        for (const c of value) {
            text.u16(c.charCodeAt(0));
        }
        w.u16(3).u16(1).u16(0x409).u16(id).u16(text.length - start).u16(start);
    }
    return w.append(text.done()).done();
}

function post(): Uint8Array {
    return new Writer().fixed(3).fixed(0).u16(-100).u16(50).u32(0).u32(0).u32(0).u32(0).u32(0).done();
}

/** One colour in a palette, as red, green, blue and alpha bytes. */
type Colour = [number, number, number, number];

const RED: Colour = [255, 0, 0, 255];
const BLUE: Colour = [0, 0, 255, 255];
const GREEN: Colour = [0, 255, 0, 255];

function cpal(palettes: Colour[][]): Uint8Array {
    const entries = palettes[0].length;
    const w = new Writer().u16(0).u16(entries).u16(palettes.length).u16(entries * palettes.length)
        .u32(12 + (palettes.length * 2));
    palettes.forEach((_, i) => w.u16(i * entries));
    for (const palette of palettes) {
        for (const [r, g, b, a] of palette) {
            w.u8(b).u8(g).u8(r).u8(a);
        }
    }
    return w.done();
}

const PALETTES = [[RED, BLUE, GREEN], [BLUE, RED, GREEN]];

/** A version 0 COLR table: each base glyph's layers, as glyph and palette entry. */
function colr0(bases: { glyph: number; layers: [number, number][] }[]): Uint8Array {
    const layers = bases.flatMap((b) => b.layers);
    const w = new Writer().u16(0).u16(bases.length).u32(14).u32(14 + (bases.length * 6)).u16(layers.length);
    let first = 0;
    for (const base of bases) {
        w.u16(base.glyph).u16(first).u16(base.layers.length);
        first += base.layers.length;
    }
    for (const [glyph, entry] of layers) {
        w.u16(glyph).u16(entry);
    }
    return w.done();
}

/**
 * A COLR version 1 paint, written by its own function into a writer. Every offset a paint holds
 * is from the paint's own start, and every paint it points to is written after it.
 */
type Paint = (w: Writer) => void;

/** Write a paint whose last field is an offset to another paint, then that paint. */
function pointing(head: (w: Writer) => number, child: Paint): Paint {
    return (w) => {
        const start = w.length;
        const at = head(w);
        w.put24(at, w.length - start);
        child(w);
    };
}

function solid(entry: number, alpha = 1): Paint {
    return (w) => {
        w.u8(2).u16(entry).f2dot14(alpha);
    };
}

/** A colour line that pads, from one palette entry at stop 0 to another at stop 1. */
function colourLine(w: Writer, from: number, to: number): void {
    w.u8(0).u16(2).f2dot14(0).u16(from).f2dot14(1).f2dot14(1).u16(to).f2dot14(1);
}

/** A paint with a colour line after its fixed fields, which the offset at 1 points to. */
function gradient(format: number, fields: (w: Writer) => void): Paint {
    return (w) => {
        const start = w.length;
        w.u8(format).u24(0);
        fields(w);
        w.put24(start + 1, w.length - start);
        colourLine(w, 0, 1);
    };
}

function linear(x0: number, y0: number, x1: number, y1: number, x2: number, y2: number): Paint {
    return gradient(4, (w) => {
        w.u16(x0).u16(y0).u16(x1).u16(y1).u16(x2).u16(y2);
    });
}

function radial(x0: number, y0: number, r0: number, x1: number, y1: number, r1: number): Paint {
    return gradient(6, (w) => {
        w.u16(x0).u16(y0).u16(r0).u16(x1).u16(y1).u16(r1);
    });
}

/** A sweep between two angles in degrees, counter-clockwise from the positive x axis. */
function sweep(cx: number, cy: number, start: number, end: number): Paint {
    return gradient(8, (w) => {
        w.u16(cx).u16(cy).f2dot14((start / 180) - 1).f2dot14((end / 180) - 1);
    });
}

function paintGlyph(glyph: number, fill: Paint): Paint {
    return pointing((w) => {
        const at = w.length + 1;
        w.u8(10).u24(0).u16(glyph);
        return at;
    }, fill);
}

function translate(dx: number, dy: number, child: Paint): Paint {
    return (w) => {
        const start = w.length;
        w.u8(14).u24(0).u16(dx).u16(dy);
        w.put24(start + 1, w.length - start);
        child(w);
    };
}

function colrLayers(first: number, count: number): Paint {
    return (w) => {
        w.u8(1).u8(count).u32(first);
    };
}

/** A version 1 COLR table: each base glyph's paint, and the layers PaintColrLayers names. */
function colr1(bases: { glyph: number; paint: Paint }[], layers: Paint[]): Uint8Array {
    const w = new Writer().u16(1).u16(0).u32(0).u32(0).u16(0);
    const baseListAt = w.length;
    w.u32(0);
    const layerListAt = w.length;
    w.u32(0).u32(0).u32(0).u32(0);

    const baseList = w.length;
    w.put32(baseListAt, baseList);
    w.u32(bases.length);
    const offsets = bases.map((base) => {
        w.u16(base.glyph);
        const at = w.length;
        w.u32(0);
        return at;
    });
    bases.forEach((base, i) => {
        w.put32(offsets[i], w.length - baseList);
        base.paint(w);
    });

    const layerList = w.length;
    w.put32(layerListAt, layerList);
    w.u32(layers.length);
    const layerOffsets = layers.map(() => {
        const at = w.length;
        w.u32(0);
        return at;
    });
    layers.forEach((layer, i) => {
        w.put32(layerOffsets[i], w.length - layerList);
        layer(w);
    });
    return w.done();
}

/** A 32 pixel square PNG, red on its left half and blue on its right. */
function png(): Uint8Array {
    const rows: number[] = [];
    for (let y = 0; y < STRIKE; y++) {
        rows.push(0);
        for (let x = 0; x < STRIKE; x++) {
            rows.push(...(x < STRIKE / 2 ? RED : BLUE));
        }
    }
    const chunk = (type: string, data: Uint8Array): Uint8Array => {
        const body = new Writer().tag(type).append(data).done();
        return new Writer().u32(data.length).append(body).u32(crc32(body)).done();
    };
    const header = new Writer().u32(STRIKE).u32(STRIKE).u8(8).u8(6).u8(0).u8(0).u8(0).done();
    return new Writer().append(Uint8Array.from([0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A]))
        .append(chunk('IHDR', header)).append(chunk('IDAT', deflateSync(Uint8Array.from(rows))))
        .append(chunk('IEND', new Uint8Array(0))).done();
}

/** The strike's pixels above the baseline, rounded. */
const STRIKE_ASCENT = Math.round((ASCENDER * STRIKE) / UNITS);

/** CBLC and CBDT for one strike holding one glyph, in format 17. */
function cbdt(glyph: number): { cblc: Uint8Array; cbdt: Uint8Array } {
    const image = png();
    const data = new Writer().u16(3).u16(0);
    const imageAt = data.length;
    data.u8(STRIKE).u8(STRIKE).u8(0).u8(STRIKE_ASCENT).u8(STRIKE).u32(image.length).append(image);

    const lineMetrics = (w: Writer): void => {
        w.u8(STRIKE_ASCENT).u8(STRIKE_ASCENT - STRIKE).u8(STRIKE).u8(1).u8(0).u8(0)
            .u8(0).u8(0).u8(STRIKE_ASCENT).u8(STRIKE_ASCENT - STRIKE).u8(0).u8(0);
    };
    const arrayAt = 8 + 48;
    const subtable = new Writer().u16(1).u16(17).u32(imageAt).u32(0).u32(data.length - imageAt).done();
    const w = new Writer().u16(3).u16(0).u32(1);
    w.u32(arrayAt).u32(8 + subtable.length).u32(1).u32(0);
    lineMetrics(w);
    lineMetrics(w);
    w.u16(glyph).u16(glyph).u8(STRIKE).u8(STRIKE).u8(32).u8(1);
    w.u16(glyph).u16(glyph).u32(8).append(subtable);
    return { cblc: w.done(), cbdt: data.done() };
}

/** An sbix table with one strike, giving one glyph a PNG. */
function sbix(glyph: number, count: number): Uint8Array {
    const image = png();
    const data = new Writer().u16(0).u16(Math.round((DESCENDER * STRIKE) / UNITS)).tag('png ').append(image).done();
    // each glyph's data runs from its offset to the next one's, so every other glyph holds none
    const start = 4 + ((count + 1) * 4);
    const strike = new Writer().u16(STRIKE).u16(72);
    for (let i = 0; i <= count; i++) {
        strike.u32(i <= glyph ? start : start + data.length);
    }
    strike.append(data);
    return new Writer().u16(1).u16(1).u32(1).u32(12).append(strike.done()).done();
}

/** An SVG table with one document serving a run of glyphs. */
function svg(first: number, last: number, document: string): Uint8Array {
    const text = new TextEncoder().encode(document);
    return new Writer().u16(0).u32(10).u32(0).u16(1).u16(first).u16(last).u32(14).u32(text.length).append(text).done();
}

function checksum(table: Uint8Array): number {
    const padded = new Uint8Array(Math.ceil(table.length / 4) * 4);
    padded.set(table);
    let sum = 0;
    for (let i = 0; i < padded.length; i += 4) {
        sum = (sum + ((padded[i] << 24 | padded[i + 1] << 16 | padded[i + 2] << 8 | padded[i + 3]) >>> 0)) >>> 0;
    }
    return sum;
}

/** A TrueType font of the given glyphs, with its colour tables. */
function font(family: string, glyphs: Glyph[], colour: Record<string, Uint8Array>): Uint8Array {
    const loca = new Writer();
    const outlinesData = new Writer();
    for (const glyph of glyphs) {
        loca.u32(outlinesData.length);
        outlinesData.append(glyf(glyph.box));
    }
    loca.u32(outlinesData.length);

    const tables: Record<string, Uint8Array> = {
        ...colour,
        'OS/2': os2(glyphs),
        cmap: cmap(glyphs),
        glyf: outlinesData.done(),
        head: head(),
        hhea: hhea(glyphs.length),
        hmtx: hmtx(glyphs),
        loca: loca.done(),
        maxp: maxp(glyphs.length),
        name: name(family),
        post: post(),
    };
    const tags = Object.keys(tables).sort();
    const count = tags.length;
    const power = 2 ** Math.floor(Math.log2(count));
    const w = new Writer().u32(0x00010000).u16(count).u16(power * 16).u16(Math.log2(power)).u16((count - power) * 16);
    const directory = 12 + (count * 16);
    let offset = directory;
    const placed = tags.map((tag) => {
        const at = offset;
        offset += Math.ceil(tables[tag].length / 4) * 4;
        return at;
    });
    tags.forEach((tag, i) => {
        w.tag(tag.padEnd(4, ' ')).u32(checksum(tables[tag])).u32(placed[i]).u32(tables[tag].length);
    });
    let headAt = 0;
    tags.forEach((tag, i) => {
        if (tag === 'head') {
            headAt = placed[i];
        }
        w.append(tables[tag]).pad();
    });
    w.put32(headAt + 8, (0xB1B0AFBA - checksum(w.done())) >>> 0);
    return w.done();
}

const here = import.meta.dirname;

function write(file: string, bytes: Uint8Array): void {
    writeFileSync(path.join(here, file), bytes);
}

{
    const glyphs = [...outlines(), { name: 'halves', box: SQUARE, code: FIRST }, { name: 'foreground', box: SQUARE, code: FIRST + 1 }];
    write('colr0.ttf', font('V3D Colour COLR0', glyphs, {
        COLR: colr0([{ glyph: 4, layers: [[2, 0], [3, 1]] }, { glyph: 5, layers: [[2, 0], [3, FOREGROUND]] }]),
        CPAL: cpal(PALETTES),
    }));
}

{
    const glyphs = [...outlines(),
        { name: 'linear', box: SQUARE, code: FIRST },
        { name: 'layers', box: SQUARE, code: FIRST + 1 },
        { name: 'radial', box: SQUARE, code: FIRST + 2 },
        { name: 'sweep', box: SQUARE, code: FIRST + 3 },
        { name: 'moved', box: SQUARE, code: FIRST + 4 }];
    const middle = (ASCENDER + DESCENDER) / 2;
    write('colr1.ttf', font('V3D Colour COLR1', glyphs, {
        COLR: colr1([
            { glyph: 4, paint: paintGlyph(1, linear(0, middle, UNITS, middle, 0, middle + UNITS)) },
            { glyph: 5, paint: colrLayers(0, 2) },
            { glyph: 6, paint: paintGlyph(1, radial(UNITS / 2, middle, 0, UNITS / 2, middle, UNITS / 2)) },
            { glyph: 7, paint: paintGlyph(1, sweep(UNITS / 2, middle, 0, 360)) },
            { glyph: 8, paint: translate(UNITS / 2, 0, paintGlyph(2, solid(2))) },
        ], [paintGlyph(2, solid(0)), paintGlyph(3, solid(FOREGROUND))]),
        CPAL: cpal(PALETTES),
    }));
}

{
    const glyphs = [...outlines(), { name: 'halves', box: SQUARE, code: FIRST }, { name: 'gradient', box: SQUARE, code: FIRST + 1 }];
    const document = [
        '<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">',
        '<defs>',
        `<rect id="half" x="0" y="${-ASCENDER}" width="${UNITS / 2}" height="${UNITS}"/>`,
        `<linearGradient id="ramp" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2="${UNITS}" y2="0">`,
        '<stop offset="0" stop-color="#ff0000"/><stop offset="1" stop-color="#0000ff"/>',
        '</linearGradient>',
        '</defs>',
        '<g id="glyph4"><use xlink:href="#half" fill="#ff0000"/><use xlink:href="#half" x="500" fill="#0000ff"/></g>',
        `<rect id="glyph5" x="0" y="${-ASCENDER}" width="${UNITS}" height="${UNITS}" fill="url(#ramp)"/>`,
        '</svg>',
    ].join('');
    write('svg.ttf', font('V3D Colour SVG', glyphs, { 'SVG ': svg(4, 5, document) }));
}

{
    const glyphs = [...outlines(), { name: 'halves', box: SQUARE, code: FIRST }];
    const tables = cbdt(4);
    write('cbdt.ttf', font('V3D Colour CBDT', glyphs, { CBDT: tables.cbdt, CBLC: tables.cblc }));
    write('sbix.ttf', font('V3D Colour SBIX', glyphs, { sbix: sbix(4, glyphs.length) }));
}
