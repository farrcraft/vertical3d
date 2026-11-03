/**
 * Tests for scripts/docs.ts, the script that builds the documentation site.
 *
 * The tests check the pinned tools and the helpers that choose and check an archive. They fetch
 * nothing. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { archiveFor, commentFindings, sha256, tool, TOOLS } from '../docs.ts';
import { formatFinding } from '../lib/cli.ts';
import { GateError } from '../lib/git.ts';
import { ROOT } from '../lib/tree.ts';

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

test('docs pins mdBook and Doxygen, and refuses a tool it does not pin', () => {
    assert.equal(tool('mdbook').name, 'mdbook');
    assert.equal(tool('doxygen').name, 'doxygen');
    assert.throws(() => tool('sphinx'), GateError);
});

test('docs refuses a platform a tool has no archive for', () => {
    const [tool] = TOOLS;
    assert.ok(tool !== undefined);
    assert.throws(() => archiveFor(tool, 'darwin-arm64'), GateError);
});

const WARNINGS = [
    `${ROOT}/api/grid/Grid.h:12: warning: Compound v3d::grid::Grid is not documented.`,
    `${ROOT}/api/grid/Grid.h:40: warning: The following parameters of v3d::grid::Grid::at(int x, int y) are not documented:`,
    '  parameter \'x\'',
    '  parameter \'y\'',
    `${ROOT}/api/ui/Prompt.h:62: warning: argument 'names' of command @param is not found in the argument list`,
    'warning: No output formats selected! Set at least one of the main GENERATE_* options to YES.',
].join('\n');

test('docs reads each warning of the reference build, with its lines joined and its path from the root', () => {
    assert.deepEqual(commentFindings(WARNINGS, false).map(formatFinding), [
        'api/grid/Grid.h:12: undocumented-type: Compound v3d::grid::Grid is not documented.',
        'api/grid/Grid.h:40: missing-param: The following parameters of v3d::grid::Grid::at(int x, int y) are not '
            + 'documented: parameter \'x\' parameter \'y\'',
        'api/ui/Prompt.h:62: comment-error: argument \'names\' of command @param is not found in the argument list',
    ]);
});

test('docs keeps only the undocumented types from the check run, which repeats the rest', () => {
    assert.deepEqual(commentFindings(WARNINGS, true).map((finding) => finding.rule), ['undocumented-type']);
});

test('docs gives the SHA-256 of some bytes in lowercase hex', () => {
    assert.equal(sha256(new TextEncoder().encode('abc')), 'ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad');
});
