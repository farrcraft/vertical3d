/**
 * Tests for scripts/kernel.ts, the check for what moya's kernel code may not do.
 *
 * Each test feeds the gate the text of kernel files, in a shape each rule has to see or one it
 * must not mistake for a breach. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { allowed, findings, uncompiled } from '../kernel.ts';

const FILE = 'moya/libmoya/kernel/Example.h';

/** The findings for one kernel file's text, as "line: rule" strings. */
function kernel(text: string): string[] {
    return findings(FILE, text).map((f) => `${f.line}: ${f.rule}`);
}

test('kernel reports a virtual function', () => {
    assert.deepEqual(kernel('struct A {\n    virtual float f() const;\n};\n'), ['2: virtual']);
});

test('kernel reports new, delete, malloc and an allocating type', () => {
    const text = [
        'float* a = new float[4];',
        'delete[] a;',
        'void* b = malloc(16);',
        'std::vector<float> c;',
        'boost::shared_ptr<int> d;',
        'std::function<void()> e;',
    ].join('\n');
    assert.deepEqual(kernel(text), ['1: allocation', '2: allocation', '3: allocation', '4: allocation', '5: allocation', '6: allocation']);
});

test('kernel does not report a deleted function', () => {
    assert.deepEqual(kernel('struct A {\n    A(const A&) = delete;\n    A& operator=(const A&) =delete;\n};\n'), []);
});

test('kernel reports throw, try and catch', () => {
    assert.deepEqual(kernel('void f() {\n    try {\n        throw 1;\n    } catch (...) {\n    }\n}\n'), [
        '2: exception',
        '3: exception',
        '4: exception',
    ]);
});

test('kernel reports dynamic_cast and typeid', () => {
    assert.deepEqual(kernel('auto* b = dynamic_cast<B*>(a);\nauto& t = typeid(a);\n'), ['1: rtti', '2: rtti']);
});

test('kernel does not read comments or literals', () => {
    const text = '// a virtual call costs a GPU too much\n/* new\n   throw */\nconst char* s = "std::vector";\n';
    assert.deepEqual(kernel(text), []);
});

test('kernel reports an include of anything but kernel, glm and the listed standard headers', () => {
    const text = [
        '#include <moya/libmoya/kernel/Qualifiers.h>',
        '#include <glm/vec3.hpp>',
        '#include <cmath>',
        '#include <cstdint>',
        '#include <vector>',
        '#include <api/type/Random.h>',
        '#include "Local.h"',
        '#include <boost/shared_ptr.hpp>',
    ].join('\n');
    assert.deepEqual(kernel(text), ['5: include', '6: include', '7: include', '8: include']);
});

test('kernel does not read an include inside a comment', () => {
    assert.deepEqual(kernel('// #include <vector>\n/*\n#include <string>\n*/\n'), []);
});

test("kernel allows only kernel paths, glm, NanoVDB's grid header and the standard headers listed", () => {
    assert.ok(allowed('moya/libmoya/kernel/spectrum/Spectrum.h'));
    assert.ok(allowed('glm/mat4x4.hpp'));
    assert.ok(allowed('nanovdb/NanoVDB.h'));
    assert.ok(allowed('type_traits'));
    assert.ok(!allowed('moya/libmoya/scene/Scene.h'));
    assert.ok(!allowed('nanovdb/tools/CreateNanoGrid.h'));
    assert.ok(!allowed('utility'));
});

test('kernel reports a header that no kernel source includes', () => {
    const files = new Map([
        ['moya/libmoya/kernel/Kernel.cxx', '#include <moya/libmoya/kernel/Qualifiers.h>\n'],
        ['moya/libmoya/kernel/Qualifiers.h', '#pragma once\n'],
        ['moya/libmoya/kernel/Spectrum.h', '#pragma once\n#include <moya/libmoya/kernel/Lost.h>\n'],
        ['moya/libmoya/kernel/Lost.h', '#pragma once\n'],
    ]);
    // a header reached only through another header is not compiled under the kernel's rules
    assert.deepEqual(
        uncompiled(files).map((f) => `${f.path}: ${f.rule}`),
        ['moya/libmoya/kernel/Spectrum.h: uncompiled', 'moya/libmoya/kernel/Lost.h: uncompiled'],
    );
});

test('kernel does not count an include a source comments out', () => {
    const files = new Map([
        ['moya/libmoya/kernel/Kernel.cxx', '// #include <moya/libmoya/kernel/Qualifiers.h>\n'],
        ['moya/libmoya/kernel/Qualifiers.h', '#pragma once\n'],
    ]);
    assert.deepEqual(
        uncompiled(files).map((f) => f.path),
        ['moya/libmoya/kernel/Qualifiers.h'],
    );
});
