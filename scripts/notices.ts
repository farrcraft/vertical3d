/**
 * Check that everything a release ships has a licence to show, and write the notices file that
 * shows them.
 *
 * A release ships the project's own code, the fonts under data/fonts/, and the libraries vcpkg
 * installs. The notices file holds the project's LICENSE, each font family's licence, and each
 * library's copyright file, in that order. The check reports:
 *
 *   no-copyright     a vcpkg package that installs code and has no copyright file.
 *   unlicensed-font  a font under data/fonts/ with no licence file named for its family.
 *
 * A package that installs files only under share/ ships no code, and needs no notice: vcpkg's
 * build helpers and meta-packages are such packages. A font's family is named by the licence
 * file beside it: Inter-OFL.txt covers every font whose name starts with Inter. When two
 * licence files match a font, the longer family wins.
 *
 * Usage: node scripts/notices.ts [--installed <dir>] [--out <file>]
 *
 *   --installed  the vcpkg_installed directory of a build. The default is x64-Debug's.
 *   --out        write the notices file there. It is written only when the check finds nothing.
 */

import { existsSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';

import { type Finding, options, report, runGate } from './lib/cli.ts';
import { GateError, tracked } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';

const USAGE = 'usage: node scripts/notices.ts [--installed <dir>] [--out <file>]';

const FONTS = 'data/fonts/';
const FONT_LICENCE = '-OFL.txt';
const FONT_FILE = /\.(ttf|otf)$/i;

/** One package vcpkg installed: its name, version and triplet. */
export interface Package {
    name: string;
    version: string;
    triplet: string;
}

/**
 * The packages a vcpkg status file says are installed, in name order. A feature's paragraph
 * names a package already listed, and is left out.
 */
export function installed(status: string): Package[] {
    const found: Package[] = [];
    for (const paragraph of status.split(/\r?\n\r?\n/)) {
        const field = (name: string) => new RegExp(`^${name}: (.*)$`, 'm').exec(paragraph)?.[1]?.trim() ?? '';
        if (!/^Status: install ok installed$/m.test(paragraph) || field('Feature') !== '') {
            continue;
        }
        const portVersion = field('Port-Version');
        found.push({
            name: field('Package'),
            version: portVersion === '' || portVersion === '0' ? field('Version') : `${field('Version')}#${portVersion}`,
            triplet: field('Architecture'),
        });
    }
    return found.sort((a, b) => a.name.localeCompare(b.name));
}

/** Whether a package's installed file list holds a file outside share/: whether it ships code. */
export function shipsCode(files: readonly string[], triplet: string): boolean {
    return files.some((file) => {
        const inside = file.startsWith(`${triplet}/`) ? file.slice(triplet.length + 1) : file;
        return inside !== '' && !inside.endsWith('/') && !inside.startsWith('share/');
    });
}

/** The family each font licence covers, from its file name: Inter-OFL.txt covers Inter. */
export function families(files: readonly string[]): Map<string, string> {
    const found = new Map<string, string>();
    for (const file of files) {
        const name = path.posix.basename(file);
        if (name.endsWith(FONT_LICENCE)) {
            found.set(name.slice(0, -FONT_LICENCE.length), file);
        }
    }
    return found;
}

/** The licence file that covers a font: the one whose family is the longest prefix of its name. */
export function licenceOf(font: string, covering: ReadonlyMap<string, string>): string | null {
    const name = path.posix.basename(font);
    let best: string | null = null;
    for (const family of covering.keys()) {
        if (name.startsWith(family) && (best === null || family.length > best.length)) {
            best = family;
        }
    }
    return best === null ? null : covering.get(best) ?? null;
}

/** The fonts among the files that no licence covers. */
export function unlicensedFonts(files: readonly string[]): Finding[] {
    const covering = families(files);
    return files
        .filter((file) => FONT_FILE.test(file) && licenceOf(file, covering) === null)
        .map((file) => ({ path: file, line: null, rule: 'unlicensed-font', message: `no *${FONT_LICENCE} beside it names its family` }));
}

/** One section of the notices file: a heading between rules, then the text. */
function section(heading: string, text: string): string {
    const rule = '='.repeat(78);
    return `${rule}\n${heading}\n${rule}\n\n${text.trim()}\n`;
}

/** A notice to show: what it is for, and the licence text. */
export interface Notice {
    heading: string;
    text: string;
}

/** The notices file: an opening paragraph, then each notice in the order given. */
export function compose(notices: readonly Notice[]): string {
    const opening = 'Third-party notices for Vertical3D\n\n'
        + 'Vertical3D is under the MIT licence below. It ships the fonts and the libraries that\n'
        + 'follow, each under the licence given with it.\n';
    return [opening, ...notices.map((notice) => section(notice.heading, notice.text))].join('\n');
}

/** The file list vcpkg wrote for a package, as lines, or null when there is none. */
function fileList(info: string, item: Package): string[] | null {
    const suffix = `_${item.triplet}.list`;
    const list = readdirSync(info).find((name) => name.startsWith(`${item.name}_`) && name.endsWith(suffix));
    return list === undefined ? null : readFileSync(path.join(info, list), 'utf8').split(/\r?\n/);
}

function main(): number {
    const { values } = options(USAGE, { installed: { type: 'string' }, out: { type: 'string' } });
    const root = path.resolve(ROOT, values.installed ?? 'out/build/x64-Debug/vcpkg_installed');
    const status = path.join(root, 'vcpkg', 'status');
    if (!existsSync(status)) {
        throw new GateError(`${root} holds no vcpkg install: configure a build first, or name one with --installed`);
    }

    const findings: Finding[] = [];
    const notices: Notice[] = [{ heading: 'Vertical3D', text: readFileSync(path.join(ROOT, 'LICENSE'), 'utf8') }];

    const fonts = tracked(FONTS);
    findings.push(...unlicensedFonts(fonts));
    for (const [family, file] of [...families(fonts)].sort(([a], [b]) => a.localeCompare(b))) {
        notices.push({ heading: `The ${family} fonts`, text: readFileSync(path.join(ROOT, file), 'utf8') });
    }

    for (const item of installed(readFileSync(status, 'utf8'))) {
        const files = fileList(path.join(root, 'vcpkg', 'info'), item);
        if (files !== null && !shipsCode(files, item.triplet)) {
            continue;
        }
        const copyright = path.join(root, item.triplet, 'share', item.name, 'copyright');
        if (!existsSync(copyright)) {
            findings.push({
                path: path.relative(ROOT, copyright).split(path.sep).join('/'),
                line: null,
                rule: 'no-copyright',
                message: `${item.name} ${item.version} installs code and has no copyright file`,
            });
            continue;
        }
        notices.push({ heading: `${item.name} ${item.version}`, text: readFileSync(copyright, 'utf8') });
    }

    const result = report(findings, 'ships something a release could not show a licence for.');
    if (result === 0 && values.out !== undefined) {
        writeFileSync(path.resolve(values.out), compose(notices));
    }
    return result;
}

runGate(import.meta.url, main);
