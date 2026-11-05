/**
 * Tests for scripts/notices.ts, the check that everything a release ships has a licence to show.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { formatFinding } from '../lib/cli.ts';
import { compose, families, installed, licenceOf, shipsCode, unlicensedFonts } from '../notices.ts';

const STATUS = [
    'Package: zlib\nVersion: 1.3.2\nArchitecture: x64-windows\nStatus: install ok installed',
    'Package: bzip2\nVersion: 1.0.8\nPort-Version: 6\nArchitecture: x64-windows\nStatus: install ok installed',
    'Package: bzip2\nFeature: tool\nArchitecture: x64-windows\nStatus: install ok installed',
    'Package: gone\nVersion: 1\nArchitecture: x64-windows\nStatus: purge ok not-installed',
].join('\n\n');

test('notices reads each installed package once, with its port version', () => {
    assert.deepEqual(installed(STATUS), [
        { name: 'bzip2', version: '1.0.8#6', triplet: 'x64-windows' },
        { name: 'zlib', version: '1.3.2', triplet: 'x64-windows' },
    ]);
});

test('notices reads a status file written with Windows line endings', () => {
    assert.equal(installed(STATUS.replaceAll('\n', '\r\n')).length, 2);
});

test('notices counts a package that installs a header or a library as shipping code', () => {
    assert.ok(shipsCode(['x64-windows/', 'x64-windows/include/', 'x64-windows/include/zlib.h'], 'x64-windows'));
    assert.ok(shipsCode(['x64-windows/bin/zlib1.dll'], 'x64-windows'));
});

test('notices counts a package that installs only under share/ as shipping nothing', () => {
    const helper = ['x64-windows/', 'x64-windows/share/', 'x64-windows/share/vcpkg-cmake/vcpkg-port-config.cmake', ''];
    assert.ok(!shipsCode(helper, 'x64-windows'));
});

test('notices names the family each font licence covers', () => {
    assert.deepEqual([...families(['data/fonts/Inter-OFL.txt', 'data/fonts/Inter-Regular.ttf', 'data/fonts/NotoSans-OFL.txt'])],
        [['Inter', 'data/fonts/Inter-OFL.txt'], ['NotoSans', 'data/fonts/NotoSans-OFL.txt']]);
});

test('notices gives a font the licence of the longest family its name starts with', () => {
    const covering = families(['data/fonts/NotoSans-OFL.txt', 'data/fonts/NotoSansCJK-OFL.txt']);
    assert.equal(licenceOf('data/fonts/NotoSansCJKsc-Regular.otf', covering), 'data/fonts/NotoSansCJK-OFL.txt');
    assert.equal(licenceOf('data/fonts/NotoSans-Regular.ttf', covering), 'data/fonts/NotoSans-OFL.txt');
});

test('notices accepts fonts that each have a licence', () => {
    assert.deepEqual(unlicensedFonts(['data/fonts/Inter-OFL.txt', 'data/fonts/Inter-Regular.ttf', 'data/fonts/Inter-SemiBold.ttf']), []);
});

test('notices reports a font no licence names the family of', () => {
    assert.deepEqual(unlicensedFonts(['data/fonts/Inter-OFL.txt', 'data/fonts/Roboto-Regular.ttf']).map(formatFinding),
        ['data/fonts/Roboto-Regular.ttf: unlicensed-font: no *-OFL.txt beside it names its family']);
});

test('notices writes each notice under its heading, after the opening', () => {
    const text = compose([{ heading: 'Vertical3D', text: 'MIT\n' }, { heading: 'zlib 1.3.2', text: '  zlib licence  ' }]);
    assert.match(text, /^Third-party notices for Vertical3D\n/);
    assert.ok(text.indexOf('Vertical3D\n=') < text.indexOf('zlib 1.3.2\n='));
    assert.match(text, /\nzlib 1\.3\.2\n=+\n\nzlib licence\n$/);
});
