/**
 * Tests for scripts/docs.ts, the script that builds the documentation site.
 *
 * The tests check the pinned tools and the helpers that choose and check an archive. They fetch
 * nothing. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { archiveFor, sha256, TOOLS } from '../docs.ts';
import { GateError } from '../lib/git.ts';

test('docs pins each tool for Windows and Linux on x64, by a full SHA-256', () => {
    for (const tool of TOOLS) {
        for (const platform of ['win32-x64', 'linux-x64']) {
            const archive = archiveFor(tool, platform);
            assert.match(archive.sha256, /^[0-9a-f]{64}$/);
            assert.ok(archive.url.startsWith('https://'), archive.url);
            assert.ok(archive.url.includes(tool.version), `${archive.url} names ${tool.version}`);
        }
    }
});

test('docs refuses a platform a tool has no archive for', () => {
    const [tool] = TOOLS;
    assert.ok(tool !== undefined);
    assert.throws(() => archiveFor(tool, 'darwin-arm64'), GateError);
});

test('docs gives the SHA-256 of some bytes in lowercase hex', () => {
    assert.equal(sha256(new TextEncoder().encode('abc')), 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');
});
