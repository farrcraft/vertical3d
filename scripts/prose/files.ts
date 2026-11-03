/**
 * Which files the prose gate reads, and how it reads their lines.
 */

import { readFileSync } from 'node:fs';
import { basename } from 'node:path';

const MARKDOWN_SUFFIXES = ['.md'];
const SLASH_SUFFIXES = ['.h', '.hpp', '.cpp', '.cxx', '.glsl', '.vert', '.frag', '.comp'];
const HASH_SUFFIXES = ['.cmake', '.yml', '.yaml'];
const CMD_SUFFIXES = ['.cmd', '.bat'];
const SCRIPT_SUFFIXES = ['.ts', '.js', '.mjs', '.cjs'];

// Historical records, and third-party or generated trees. ports/ holds vcpkg's own ports, overlaid.
const EXCLUDED_PREFIXES = ['vendor/', 'ports/', 'demos/voxel/src/noise/', 'out/', 'docs/plans/completed/', 'docs/roadmap/completed/',
    'docs/audits/'];
// An accepted ADR's body is not edited, so only a new ADR file is checked.
const ADR_BODY = /^docs\/adr\/\d{4}-[^/]*\.md$/;

export type Kind = 'md' | 'slash' | 'script' | 'hash' | 'cmd';

export function columns(text: string): number {
    return Array.from(text).length;
}

export function fileKind(path: string): Kind | null {
    const name = basename(path);
    const lower = name.toLowerCase();
    if (MARKDOWN_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'md';
    }
    if (SLASH_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'slash';
    }
    if (SCRIPT_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'script';
    }
    if (name === 'CMakeLists.txt' || HASH_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'hash';
    }
    if (CMD_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'cmd';
    }
    return null;
}

export function excluded(path: string, newFiles: Set<string>): boolean {
    if (EXCLUDED_PREFIXES.some((prefix) => path.startsWith(prefix)) || path.includes('/vcpkg_installed/')) {
        return true;
    }
    return ADR_BODY.test(path) && !newFiles.has(path);
}

export function readLines(path: string): string[] {
    return readFileSync(path, 'utf8').split('\n').map((line) => line.replace(/\r$/, ''));
}
