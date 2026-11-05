/**
 * Tests for scripts/changelog.ts, the check that a change to what a release ships adds its entry
 * to CHANGELOG.md.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { hasUnreleased, problems, ships } from '../changelog.ts';
import { formatFinding } from '../lib/cli.ts';

const CHANGELOG = '# Changelog\n\n## [Unreleased]\n\n### Added\n\n- a thing\n';

/** The problems with a change, as the lines the gate prints. */
function check(changed: string[], changelog = CHANGELOG): string[] {
    return problems(changed, changelog).map(formatFinding);
}

test('changelog counts the api, the apps, the demos and the shared data as shipped', () => {
    for (const file of ['api/engine/Engine.cpp', 'vertical3d/src/Controller.cxx', 'moya/moya.cxx', 'imagetool/imagetool.cxx',
        'demos/pong/src/pong.cxx', 'data/fonts/Inter-Regular.ttf', 'vertical3d/data/vgui.json']) {
        assert.ok(ships(file), file);
    }
});

test('changelog does not count tests, READMEs or anything outside the shipped directories', () => {
    for (const file of ['api/engine/tests/EngineTest.cpp', 'api/engine/README.md', 'docs/api/README.md', 'scripts/prose.ts',
        '.github/workflows/ctest.yml', 'CMakeLists.txt']) {
        assert.ok(!ships(file), file);
    }
});

test('changelog accepts a change to what ships that adds an entry', () => {
    assert.deepEqual(check(['api/engine/Engine.cpp', 'CHANGELOG.md']), []);
});

test('changelog accepts a change that ships nothing without an entry', () => {
    assert.deepEqual(check(['docs/TODO.md', 'api/engine/tests/EngineTest.cpp']), []);
});

test('changelog reports a change to what ships that adds no entry, naming what it touched', () => {
    assert.deepEqual(check(['api/engine/Engine.cpp']), ['CHANGELOG.md: no-entry: the change touches api/engine/Engine.cpp, and adds no entry']);
    assert.deepEqual(check(['api/engine/Engine.cpp', 'moya/moya.cxx']),
        ['CHANGELOG.md: no-entry: the change touches api/engine/Engine.cpp and 1 more, and adds no entry']);
});

test('changelog finds the Unreleased section, and reports a changelog without one', () => {
    assert.ok(hasUnreleased(CHANGELOG));
    assert.ok(!hasUnreleased('# Changelog\n\n## [0.1.0] - 2026-11-05\n'));
    assert.deepEqual(check([], '# Changelog\n'), ['CHANGELOG.md: no-unreleased: there is no "## [Unreleased]" section']);
});
