/**
 * Tests for scripts/namespaces.ts, the check that each C++ file is in its directory's namespace.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { expected, findings, opened } from '../namespaces.ts';

/** The findings for a text, as "line: rule" strings. */
function check(text: string, own: string, known: string[]): string[] {
    return findings(text, own, new Set(known)).map((f) => `${f.line}: ${f.rule}`);
}

test('namespaces gives a file the namespace of its root and the directories below it', () => {
    assert.equal(expected('api/asset/Asset.h'), 'v3d::asset');
    assert.equal(expected('api/render/realtime/vulkan/device/Device.cpp'), 'v3d::render::realtime::vulkan::device');
    assert.equal(expected('api/interchange/usd/internal/Reader.h'), 'v3d::interchange::usd::internal');
    assert.equal(expected('moya/libmoya/state/Attributes.h'), 'v3d::moya::state');
    assert.equal(expected('moya/moya.cxx'), 'v3d::moya');
    assert.equal(expected('vertical3d/src/Controller.h'), 'v3d::editor');
    assert.equal(expected('vertical3d/src/scene/Scene.h'), 'v3d::editor::scene');
});

test('namespaces checks no test, and no file outside a root', () => {
    assert.equal(expected('api/asset/tests/AssetTest.cpp'), null);
    assert.equal(expected('vertical3d/tests/scene/ItemTest.cxx'), null);
    assert.equal(expected('demos/odyssey/src/engine/Unit.h'), null);
    assert.equal(expected('imagetool/imagetool.cxx'), null);
});

test('namespaces names a nested namespace in full, and skips an anonymous one', () => {
    const text = [
        'namespace v3d::font {',
        'namespace {',
        '}',
        'namespace internal {',
        'class Glyph;',
        '}',
        '};  // namespace v3d::font',
    ].join('\n');
    assert.deepEqual(opened(text), [{ line: 1, name: 'v3d::font' }, { line: 4, name: 'v3d::font::internal' }]);
});

test('namespaces reads no namespace in a comment, a literal or a macro', () => {
    const text = [
        '// namespace v3d::wrong {',
        'const char* text = "namespace v3d::wrong {";',
        '#define OPEN namespace v3d::wrong {',
        'namespace v3d::right {',
        '};  // namespace v3d::right',
    ].join('\n');
    assert.deepEqual(opened(text).map((o) => o.name), ['v3d::right']);
});

test('namespaces reports a file that opens only a parent of its own namespace', () => {
    const text = 'namespace v3d::editor {\nclass Item {};\n};  // namespace v3d::editor\n';
    assert.deepEqual(check(text, 'v3d::editor::scene', ['v3d::editor', 'v3d::editor::scene']), ['1: own-namespace']);
});

test('namespaces allows a forward declaration in another directory\'s namespace', () => {
    const text = [
        'namespace v3d::editor::view {',
        'class ViewPort;',
        '};  // namespace v3d::editor::view',
        '',
        'namespace v3d::editor::tool {',
        'class Tool {};',
        '};  // namespace v3d::editor::tool',
    ].join('\n');
    assert.deepEqual(check(text, 'v3d::editor::tool', ['v3d::editor::view', 'v3d::editor::tool']), []);
});

test('namespaces reports a namespace under v3d that no directory gives', () => {
    const text = 'namespace v3d::render::offline {\nnamespace uniform {\nvoid f();\n}\n}\n';
    assert.deepEqual(check(text, 'v3d::render::offline', ['v3d::render::offline']), ['2: no-directory']);
});

test('namespaces leaves a namespace outside v3d alone, and a file that opens none', () => {
    const fmod = 'namespace FMOD::Studio {\nclass System;\n}\nnamespace v3d::audio {\n}\n';
    assert.deepEqual(check(fmod, 'v3d::audio', ['v3d::audio']), []);
    assert.deepEqual(check('int main() {\n    return 0;\n}\n', 'v3d::editor', ['v3d::editor']), []);
});
