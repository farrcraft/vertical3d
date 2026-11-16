/**
 * Tests for scripts/namespaces.ts, the check that each C++ file is in its directory's namespace.
 *
 * Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { described, descriptionFindings, expected, findings, opened } from '../namespaces.ts';

/** The findings for a text, as "line: rule" strings. */
function check(text: string, own: string, known: string[]): string[] {
    return findings(text, own, new Set(known)).map((f) => `${f.line}: ${f.rule}`);
}

/** The description findings for some headers and a namespaces.dox, as "path:line: rule" strings. */
function describe(headers: Array<[string, string, string]>, doxText = ''): string[] {
    return descriptionFindings(
        headers.map(([file, own, text]) => ({ file, own, text })),
        doxText,
    ).map((f) => `${f.path}:${f.line}: ${f.rule}`);
}

/** A header that opens a namespace, with a doc comment above the opening when described. */
function header(name: string, describedHere: boolean): string {
    const comment = describedHere ? '/**\n * What it holds.\n **/\n' : '';
    return `/*\n * Vertical3D\n */\n\n#pragma once\n\n${comment}namespace ${name} {\n};  // namespace ${name}\n`;
}

/** A namespaces.dox that describes each of some namespaces. */
function dox(...names: string[]): string {
    return names.map((name) => `/**\n * @namespace ${name}\n * What it holds.\n **/\n`).join('\n');
}

test('namespaces gives a file the namespace of its root and the directories below it', () => {
    assert.equal(expected('api/asset/Asset.h'), 'v3d::asset');
    assert.equal(expected('api/render/realtime/vulkan/device/Device.cpp'), 'v3d::render::realtime::vulkan::device');
    assert.equal(expected('api/interchange/usd/internal/Reader.h'), 'v3d::interchange::usd::internal');
    assert.equal(expected('moya/libmoya/scene/Kinds.h'), 'v3d::moya::scene');
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
    assert.deepEqual(opened(text), [
        { line: 1, name: 'v3d::font' },
        { line: 4, name: 'v3d::font::internal' },
    ]);
});

test('namespaces reads no namespace in a comment, a literal or a macro', () => {
    const text = [
        '// namespace v3d::wrong {',
        'const char* text = "namespace v3d::wrong {";',
        '#define OPEN namespace v3d::wrong {',
        'namespace v3d::right {',
        '};  // namespace v3d::right',
    ].join('\n');
    assert.deepEqual(
        opened(text).map((o) => o.name),
        ['v3d::right'],
    );
});

test('namespaces reports a file that opens only a parent of its own namespace', () => {
    const text = 'namespace v3d::editor {\nclass Item {};\n};  // namespace v3d::editor\n';
    assert.deepEqual(check(text, 'v3d::editor::scene', ['v3d::editor', 'v3d::editor::scene']), ['1: own-namespace']);
});

test("namespaces allows a forward declaration in another directory's namespace", () => {
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

test('namespaces reads a doc comment directly above an opening as a description', () => {
    assert.deepEqual(described(header('v3d::asset', true)), [{ line: 10, name: 'v3d::asset' }]);
    assert.deepEqual(described(header('v3d::asset', false)), []);
    const apart = '/**\n * A class.\n **/\nclass A;\n\nnamespace v3d::asset {\n}\n';
    assert.deepEqual(described(apart), []);
});

test('namespaces accepts each namespace described once, the parents in namespaces.dox', () => {
    const headers: Array<[string, string, string]> = [
        ['api/asset/Asset.h', 'v3d::asset', header('v3d::asset', true)],
        ['api/asset/File.h', 'v3d::asset', header('v3d::asset', false)],
        ['api/font/face/Face.h', 'v3d::font::face', header('v3d::font::face', true)],
    ];
    assert.deepEqual(describe(headers, dox('v3d', 'v3d::font')), []);
});

test('namespaces reports a namespace no header describes, on its first header', () => {
    const headers: Array<[string, string, string]> = [
        ['api/asset/Manager.h', 'v3d::asset', header('v3d::asset', false)],
        ['api/asset/File.h', 'v3d::asset', header('v3d::asset', false)],
    ];
    assert.deepEqual(describe(headers, dox('v3d')), ['api/asset/File.h:7: no-description']);
});

test('namespaces reports a parent namespace that namespaces.dox does not describe', () => {
    const headers: Array<[string, string, string]> = [['api/font/face/Face.h', 'v3d::font::face', header('v3d::font::face', true)]];
    assert.deepEqual(describe(headers, dox('v3d')), ['scripts/docs/namespaces.dox:1: no-description']);
});

test('namespaces reports a namespace described twice', () => {
    const headers: Array<[string, string, string]> = [
        ['api/asset/Asset.h', 'v3d::asset', header('v3d::asset', true)],
        ['api/asset/File.h', 'v3d::asset', header('v3d::asset', true)],
    ];
    assert.deepEqual(describe(headers, dox('v3d')), ['api/asset/File.h:10: described-twice']);
    const inDox = [['api/asset/Asset.h', 'v3d::asset', header('v3d::asset', true)]] as Array<[string, string, string]>;
    assert.deepEqual(describe(inDox, dox('v3d', 'v3d::asset')), ['scripts/docs/namespaces.dox:7: described-twice']);
});
