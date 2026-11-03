/**
 * Make the pictures the documents show, from scenes in the tree.
 *
 * Each picture in PICTURES is rendered by moya from a RIB scene in docs/, at the size the scene
 * names, and written where the document links it. A scene that uses a shader of its own names
 * no search path, because moya reads a relative path from its own directory. The script copies
 * the scene into out/pictures/ with the directory of its shaders added to the shader search path,
 * and renders the copy.
 *
 * moya has to be built first, in out/build/x64-Debug or the build that --build names. It is run
 * from its own directory, which it needs to find its libraries.
 *
 * Usage: node scripts/pictures.ts [--build <dir>]
 */

import { spawnSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { options, runGate } from './lib/cli.ts';
import { GateError } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';

/** One picture: the scene it is rendered from, the image written, and its shaders' directory. */
export interface Picture {
    scene: string;
    image: string;
    shaders?: string;
}

const MOYA = 'docs/manual/moya';

/** Every picture this script makes. Paths are relative to the root. */
export const PICTURES: readonly Picture[] = [
    { scene: `${MOYA}/first.rib`, image: `${MOYA}/images/first.png` },
    { scene: `${MOYA}/scenes/stripes.rib`, image: `${MOYA}/images/stripes.png`, shaders: `${MOYA}/scenes/shaders` },
    { scene: `${MOYA}/scenes/ripple.rib`, image: `${MOYA}/images/ripple.png`, shaders: `${MOYA}/scenes/shaders` },
    { scene: `${MOYA}/scenes/focus.rib`, image: `${MOYA}/images/focus.png` },
];

/** A scene with a directory put first on its shader search path, in moya's form of a path. */
export function withShaders(scene: string, directory: string): string {
    const forward = directory.split(path.sep).join('/');
    return `Option "searchpath" "shader" ["${forward}:&"]\n${scene}`;
}

function main(): number {
    const { values } = options('usage: node scripts/pictures.ts [--build <dir>]', { build: { type: 'string' } });
    const build = path.resolve(ROOT, values.build ?? 'out/build/x64-Debug');
    const moya = path.join(build, 'moya', process.platform === 'win32' ? 'moya.exe' : 'moya');
    if (!existsSync(moya)) {
        throw new GateError(`${moya} does not exist; build the moya target first`);
    }
    const scratch = path.join(ROOT, 'out', 'pictures');
    mkdirSync(scratch, { recursive: true });
    for (const picture of PICTURES) {
        const text = readFileSync(path.join(ROOT, picture.scene), 'utf8');
        const scene = path.join(scratch, path.basename(picture.scene));
        writeFileSync(scene, picture.shaders === undefined ? text : withShaders(text, path.join(ROOT, picture.shaders)));
        const image = path.join(ROOT, picture.image);
        mkdirSync(path.dirname(image), { recursive: true });
        // moya exits 0 when it cannot write the image, so the file is removed and looked for after
        rmSync(image, { force: true });
        const run = spawnSync(moya, ['--file', scene, '--output', image, '--silent'], { cwd: path.dirname(moya), stdio: 'inherit' });
        if (run.error !== undefined || run.status !== 0 || !existsSync(image)) {
            throw new GateError(`moya did not render ${picture.scene}`);
        }
        process.stdout.write(`${picture.image}\n`);
    }
    return 0;
}

runGate(import.meta.url, main);
