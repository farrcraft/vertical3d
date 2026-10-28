// Generate well.wav, the loop the diorama places at its well: running water, made from noise.
//
// White noise from a fixed seed is smoothed by two one-pole low-pass filters, so it rushes rather
// than hisses, and its level swells and falls with two slow waves. Each wave fits a whole number
// of times into the loop, and the last quarter second is faded across into the first, so the loop
// has no seam when it repeats.
//
// The file is 16-bit mono PCM at 22050 samples a second, four seconds long.
//
// Run from the root of the tree:
//   node demos/diorama/assets/make_sounds.ts

import { writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const RATE = 22050;
const SECONDS = 4;
const CROSSFADE = RATE / 4;
const LEVEL = 0.35;

const here = dirname(fileURLToPath(import.meta.url));
const out = join(here, '..', 'data', 'well.wav');

// a linear congruential generator, so every run writes the same file
let seed = 20261027;
function noise(): number {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
    return (seed / 0x100000000) * 2 - 1;
}

const length = RATE * SECONDS;
const total = length + CROSSFADE;
const raw = new Float64Array(total);
let first = 0;
let second = 0;
for (let index = 0; index < total; index++) {
    first += 0.25 * (noise() - first);
    second += 0.25 * (first - second);
    const time = (index % length) / RATE;
    const swell = 0.75 + 0.15 * Math.sin(2 * Math.PI * time / SECONDS) + 0.1 * Math.sin(2 * Math.PI * 3 * time / SECONDS);
    raw[index] = second * swell;
}

// the samples past the loop's end fade into its start, and the loop is the first `length`
let peak = 0;
const loop = new Float64Array(length);
for (let index = 0; index < length; index++) {
    loop[index] = raw[index];
    if (index < CROSSFADE) {
        const across = index / CROSSFADE;
        loop[index] = raw[index] * across + raw[length + index] * (1 - across);
    }
    peak = Math.max(peak, Math.abs(loop[index]));
}

const data = Buffer.alloc(length * 2);
for (let index = 0; index < length; index++) {
    data.writeInt16LE(Math.round((loop[index] / peak) * LEVEL * 32767), index * 2);
}

const header = Buffer.alloc(44);
header.write('RIFF', 0, 'ascii');
header.writeUInt32LE(36 + data.length, 4);
header.write('WAVE', 8, 'ascii');
header.write('fmt ', 12, 'ascii');
header.writeUInt32LE(16, 16);
header.writeUInt16LE(1, 20);  // PCM
header.writeUInt16LE(1, 22);  // mono
header.writeUInt32LE(RATE, 24);
header.writeUInt32LE(RATE * 2, 28);
header.writeUInt16LE(2, 32);
header.writeUInt16LE(16, 34);
header.write('data', 36, 'ascii');
header.writeUInt32LE(data.length, 40);

writeFileSync(out, Buffer.concat([header, data]));
console.log(`wrote ${out}`);
