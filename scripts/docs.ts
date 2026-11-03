/**
 * Build the documentation site from docs/ into out/docs/.
 *
 * The script fetches mdBook at the version TOOLS pins, into out/tools/. It checks the archive
 * against the pinned SHA-256 before it extracts it, and it fetches nothing when the binary is
 * already there from an archive that matched. It then builds the book that
 * scripts/docs/book.toml describes. The build stops on a link that names nothing, which
 * scripts/docs/outside.ts reports.
 *
 * It runs on Windows and on Linux, on x64. It needs Node, the network on its first run, and tar,
 * which both systems ship.
 *
 * Usage: node scripts/docs.ts
 */

import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { options, UsageError } from './lib/cli.ts';
import { shouldRun } from './lib/entry.ts';
import { GateError } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';

/** One archive of a tool: where it is fetched from, its SHA-256, and the binary inside it. */
export interface Archive {
    url: string;
    sha256: string;
    binary: string;
}

/** A tool at a pinned version, with an archive for each platform it runs on. */
export interface Tool {
    name: string;
    version: string;
    archives: Readonly<Record<string, Archive>>;
}

const MDBOOK_RELEASES = 'https://github.com/rust-lang/mdBook/releases/download/v0.5.4';
const DOXYGEN_RELEASES = 'https://github.com/doxygen/doxygen/releases/download/Release_1_18_0';

/** The tools the site is built with. */
export const TOOLS: readonly Tool[] = [
    {
        name: 'mdbook',
        version: '0.5.4',
        archives: {
            'win32-x64': {
                url: `${MDBOOK_RELEASES}/mdbook-v0.5.4-x86_64-pc-windows-msvc.zip`,
                sha256: '8a6b2421aa522de06d746871d2b8fff9c8d71773467e310a4516d2566e7f2de4',
                binary: 'mdbook.exe',
            },
            'linux-x64': {
                url: `${MDBOOK_RELEASES}/mdbook-v0.5.4-x86_64-unknown-linux-musl.tar.gz`,
                sha256: '5222beabd3e37dc5be0d18ff99b79058469354db5c220153a1b92db5ba12be89',
                binary: 'mdbook',
            },
        },
    },
    {
        name: 'doxygen',
        version: '1.18.0',
        archives: {
            'win32-x64': {
                url: `${DOXYGEN_RELEASES}/doxygen-1.18.0.windows.x64.bin.zip`,
                sha256: 'e84f54cfd49ef06b0b16536056dbec0c496323de28abcce53a4269463de35eaf',
                binary: 'doxygen.exe',
            },
            'linux-x64': {
                url: `${DOXYGEN_RELEASES}/doxygen-1.18.0.linux.bin.tar.gz`,
                sha256: '14fa81bdc34171edb5f1f02b1d60e74802f0439b77fa44e592565d517d72df90',
                binary: 'doxygen-1.18.0/bin/doxygen',
            },
        },
    },
];

/** The archive of a tool for this platform. Throws a GateError when the tool has none. */
export function archiveFor(tool: Tool, platform: string): Archive {
    const archive = tool.archives[platform];
    if (archive === undefined) {
        throw new GateError(`${tool.name} ${tool.version} has no archive for ${platform}`);
    }
    return archive;
}

/** The tool TOOLS pins by a name. Throws a GateError when it pins none. */
export function tool(name: string): Tool {
    const found = TOOLS.find((candidate) => candidate.name === name);
    if (found === undefined) {
        throw new GateError(`TOOLS pins no ${name}`);
    }
    return found;
}

/** The SHA-256 of some bytes, in lowercase hex. */
export function sha256(bytes: Uint8Array): string {
    return createHash('sha256').update(bytes).digest('hex');
}

/** Runs a command and throws a GateError when it fails. */
function run(command: string, args: readonly string[], cwd: string): void {
    const result = spawnSync(command, args, { cwd, stdio: 'inherit' });
    if (result.error !== undefined || result.status !== 0) {
        throw new GateError(`${command} ${args.join(' ')} failed`);
    }
}

/**
 * The path of a tool's binary, fetched and extracted first when it is not there. A file beside
 * the binary records the SHA-256 of the archive it came from.
 */
async function ensure(tool: Tool, platform: string): Promise<string> {
    const archive = archiveFor(tool, platform);
    const directory = path.join(ROOT, 'out', 'tools', `${tool.name}-${tool.version}`);
    const binary = path.join(directory, archive.binary);
    const stamp = path.join(directory, 'sha256');
    if (existsSync(binary) && existsSync(stamp) && readFileSync(stamp, 'utf8') === archive.sha256) {
        return binary;
    }
    const response = await fetch(archive.url);
    if (!response.ok) {
        throw new GateError(`fetching ${archive.url} answered ${response.status}`);
    }
    const bytes = new Uint8Array(await response.arrayBuffer());
    const actual = sha256(bytes);
    if (actual !== archive.sha256) {
        throw new GateError(`${archive.url} has SHA-256 ${actual}, and ${archive.sha256} is pinned`);
    }
    mkdirSync(directory, { recursive: true });
    const file = path.join(directory, path.posix.basename(archive.url));
    writeFileSync(file, bytes);
    // Windows' own tar reads a zip; the tar on PATH under Git Bash does not
    const tar = platform.startsWith('win32')
        ? path.join(process.env['SystemRoot'] ?? 'C:\\Windows', 'System32', 'tar.exe')
        : 'tar';
    run(tar, ['-xf', file, '-C', directory], directory);
    writeFileSync(stamp, archive.sha256);
    return binary;
}

async function main(): Promise<number> {
    // the script takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/docs.ts', {});
    const platform = `${process.platform}-${process.arch}`;
    const book = path.join(ROOT, 'scripts', 'docs');
    const mdbook = await ensure(tool('mdbook'), platform);
    const doxygen = await ensure(tool('doxygen'), platform);
    // mdBook empties out/docs/ before it writes, so the reference is built after it
    run(mdbook, ['build'], book);
    run(doxygen, ['Doxyfile'], book);
    return 0;
}

// runGate takes a main that returns at once, and this one waits on the network
if (shouldRun(import.meta.url)) {
    main().then((status) => {
        process.exitCode = status;
    }, (error: unknown) => {
        if (!(error instanceof GateError)) {
            throw error;
        }
        process.stderr.write(error instanceof UsageError ? `${error.message}\n` : `docs.ts: ${error.message}\n`);
        process.exitCode = 2;
    });
}
