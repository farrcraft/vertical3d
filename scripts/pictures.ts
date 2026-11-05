/**
 * Make the pictures the documents show, from scenes and scripts in the tree.
 *
 * A picture is one of two kinds. A render is a RIB scene in docs/ that moya renders, at the size
 * the scene names. A scene that uses a shader of its own names no search path, because moya reads
 * a relative path from its own directory. The script copies the scene into out/pictures/ with the
 * directory of its shaders added to the shader search path, and renders the copy. A capture is an
 * app started with --capture, which writes one frame of its window and quits. The editor is also
 * given a script that builds the scene shown.
 *
 * The programs have to be built first, in out/build/x64-Debug or the build that --build names.
 * Each is run from its own directory, which moya needs to find its libraries. A capture opens the
 * app's window on the desktop for the seconds it runs.
 *
 * Usage: node scripts/pictures.ts [--build <dir>] [--only <image>]
 */

import { spawnSync } from 'node:child_process';
import { existsSync, mkdirSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { options, runGate } from './lib/cli.ts';
import { GateError } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';

/** A picture moya renders from a scene, with the directory of the scene's own shaders. */
export interface Render {
    kind: 'render';
    image: string;
    scene: string;
    shaders?: string;
}

/** A picture an app writes of its own window: the app's path in the build, and what it is given. */
export interface Capture {
    kind: 'capture';
    image: string;
    app: string;
    after: number;
    script?: string;
}

export type Picture = Render | Capture;

const MOYA = 'docs/manual/moya';
const EDITOR = 'docs/manual/editor';

/** Every picture this script makes. Paths are relative to the root, and an app's to the build. */
export const PICTURES: readonly Picture[] = [
    { kind: 'render', image: `${MOYA}/images/first.png`, scene: `${MOYA}/first.rib` },
    { kind: 'render', image: `${MOYA}/images/stripes.png`, scene: `${MOYA}/scenes/stripes.rib`, shaders: `${MOYA}/scenes/shaders` },
    { kind: 'render', image: `${MOYA}/images/ripple.png`, scene: `${MOYA}/scenes/ripple.rib`, shaders: `${MOYA}/scenes/shaders` },
    { kind: 'render', image: `${MOYA}/images/focus.png`, scene: `${MOYA}/scenes/focus.rib` },
    { kind: 'capture', image: `${EDITOR}/images/first-scene.png`, app: 'vertical3d/vertical3d', after: 60, script: `${EDITOR}/scenes/first-scene.ts` },
    { kind: 'capture', image: 'docs/images/pong.png', app: 'demos/pong/pong', after: 120 },
    { kind: 'capture', image: 'docs/images/tetris.png', app: 'demos/tetris/tetris', after: 120 },
    { kind: 'capture', image: 'docs/images/voxel.png', app: 'demos/voxel/voxel', after: 300 },
    { kind: 'capture', image: 'docs/images/odyssey.png', app: 'demos/odyssey/odyssey', after: 120 },
    { kind: 'capture', image: 'docs/images/diorama.png', app: 'demos/diorama/diorama', after: 240 },
];

/** A scene with a directory put first on its shader search path, in moya's form of a path. */
export function withShaders(scene: string, directory: string): string {
    const forward = directory.split(path.sep).join('/');
    return `Option "searchpath" "shader" ["${forward}:&"]\n${scene}`;
}

/** The arguments an app is started with to write a capture to image, a path from the root. */
export function captureArguments(picture: Capture): string[] {
    const script = picture.script === undefined ? [] : ['--script', path.join(ROOT, picture.script)];
    return [...script, '--capture', path.join(ROOT, picture.image), '--capture-after', String(picture.after)];
}

/** An executable's path in the build, with the extension the platform gives one. */
function executable(build: string, app: string): string {
    const file = path.join(build, ...app.split('/'));
    return process.platform === 'win32' ? `${file}.exe` : file;
}

/** Runs one program from its own directory, and throws a GateError unless it wrote the image. */
function make(program: string, args: readonly string[], image: string, what: string): void {
    if (!existsSync(program)) {
        throw new GateError(`${program} does not exist; build it first`);
    }
    mkdirSync(path.dirname(image), { recursive: true });
    // a program can exit 0 without writing the image, so the file is removed and looked for after
    rmSync(image, { force: true });
    const run = spawnSync(program, args, { cwd: path.dirname(program), stdio: 'inherit' });
    if (run.error !== undefined || run.status !== 0 || !existsSync(image)) {
        throw new GateError(`${what} did not write ${path.relative(ROOT, image)}`);
    }
}

function main(): number {
    const { values } = options('usage: node scripts/pictures.ts [--build <dir>] [--only <image>]', {
        build: { type: 'string' },
        only: { type: 'string' },
    });
    const build = path.resolve(ROOT, values.build ?? 'out/build/x64-Debug');
    const chosen = PICTURES.filter((picture) => values.only === undefined || picture.image === values.only);
    if (chosen.length === 0) {
        throw new GateError(`no picture is written to ${values.only ?? ''}`);
    }
    const scratch = path.join(ROOT, 'out', 'pictures');
    mkdirSync(scratch, { recursive: true });
    for (const picture of chosen) {
        const image = path.join(ROOT, picture.image);
        if (picture.kind === 'render') {
            const text = readFileSync(path.join(ROOT, picture.scene), 'utf8');
            const scene = path.join(scratch, path.basename(picture.scene));
            writeFileSync(scene, picture.shaders === undefined ? text : withShaders(text, path.join(ROOT, picture.shaders)));
            make(executable(build, 'moya/moya'), ['--file', scene, '--output', image, '--silent'], image, `moya on ${picture.scene}`);
        } else {
            make(executable(build, picture.app), captureArguments(picture), image, picture.app);
        }
        process.stdout.write(`${picture.image}\n`);
    }
    return 0;
}

runGate(import.meta.url, main);
