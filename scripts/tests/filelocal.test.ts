/**
 * Tests for scripts/filelocal.ts, the check of what a C++ file keeps to itself.
 *
 * Each test feeds the check a shape of C++ that breaks one rule, or a shape that looks like a
 * break and is not. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { test } from 'node:test';

import { externNames, findings, kindOf } from '../filelocal.ts';

/** The findings for the lines of a file at a path, as "line: rule" strings. */
function check(file: string, lines: string[], externs: string[] = []): string[] {
    return findings(lines.join('\n'), kindOf(file, new Set(externs))).map((f) => `${f.line}: ${f.rule}`);
}

const SOURCE = 'api/example/Example.cxx';
const TEST = 'api/example/tests/ExampleTest.cxx';

test('filelocal accepts one anonymous namespace at the top, holding constants and functions', () => {
    assert.deepEqual(check(SOURCE, [
        '#include "Example.h"',
        'namespace v3d::example {',
        'namespace {',
        'constexpr float HALF = 0.5F;',
        'const char* const NAME = "name";',
        'float half(float x) {',
        '    return x * HALF;',
        '}',
        '};  // namespace',
        'float Example::size() const {',
        '    return half(size_);',
        '}',
        '};  // namespace v3d::example',
    ]), []);
});

test('filelocal reports a type defined in a source file, but not in a header or a test', () => {
    const lines = [
        'namespace {',
        'struct Form {',
        '    int type;',
        '};',
        'class Reader final : public Base<int> {',
        ' public:',
        '    void read() {}',
        '};',
        '};  // namespace',
    ];
    assert.deepEqual(check(SOURCE, lines), ['2: type-in-source', '5: type-in-source']);
    assert.deepEqual(check('api/example/Example.h', lines), []);
    assert.deepEqual(check(TEST, lines), []);
});

test('filelocal reports a declaration below a function, and an anonymous namespace below one', () => {
    assert.deepEqual(check(SOURCE, [
        'namespace {',
        'void first() {',
        '}',
        'constexpr int LATE = 1;',
        'struct Late {};',
        'using Alias = int;',
        '};  // namespace',
        'namespace {',
        '};  // namespace',
    ]), ['4: after-function', '5: type-in-source', '5: after-function', '6: after-function', '8: second-anonymous', '8: anonymous-late']);
});

test('filelocal treats a test case as a function, so a fixture below one is reported', () => {
    assert.deepEqual(check(TEST, [
        'namespace {',
        'struct Fixture {};',
        'BOOST_AUTO_TEST_SUITE(example)',
        'BOOST_AUTO_TEST_CASE(first) {',
        '    BOOST_TEST(true);',
        '}',
        'const int AFTER = 2;',
        'BOOST_AUTO_TEST_SUITE_END()',
        '};  // namespace',
    ]), ['7: after-function']);
});

test('filelocal reports a mutable variable, a mutable pointer and a mutable static member', () => {
    assert.deepEqual(check(SOURCE, [
        'namespace {',
        'unsigned int lastId = 0;',
        'const char* NAME = "name";',
        'std::atomic<unsigned int> next { 0 };',
        'std::atomic<std::uint64_t> serial(1);',
        'int make(int value);',
        '};  // namespace',
        'int Example::count = 0;',
    ]), ['2: mutable-variable', '3: mutable-variable', '4: mutable-variable', '5: mutable-variable', '8: mutable-variable']);
});

test('filelocal reads constness from the variable, not from its template arguments', () => {
    assert.deepEqual(check(SOURCE, [
        'namespace {',
        'const std::array<const char*, 3> AXES{ "x", "y", "z" };',
        'const std::pair<const char*, int> MOVES[] = { { "a", 1 } };',
        'template <typename T>',
        'concept Sized = requires(T t) { t.size(); };',
        '};  // namespace',
    ]), []);
});

test('filelocal reports a constant in a source file outside the anonymous namespace', () => {
    assert.deepEqual(check(SOURCE, [
        'namespace v3d::example {',
        'constexpr int OUTSIDE = 3;',
        '};  // namespace v3d::example',
    ]), ['2: outside-anonymous']);
});

test('filelocal accepts a variable that a header declares extern', () => {
    const header = 'extern RtToken RI_FRAMEBUFFER, RI_FILE;\nextern RtInt RiLastError;\nextern "C" {\n}';
    const externs = externNames(header);
    assert.deepEqual(externs, ['RI_FRAMEBUFFER', 'RI_FILE', 'RiLastError']);
    assert.deepEqual(check(SOURCE, [
        'void f() {',
        '}',
        'RtToken RI_FRAMEBUFFER = const_cast<char*>("framebuffer"), RI_FILE = const_cast<char*>("file");',
        'RtInt RiLastError;',
    ], externs), []);
});

test('filelocal reports a mutable static local, unless it is marked process-wide', () => {
    assert.deepEqual(check(SOURCE, [
        'Renderer& renderer() {',
        '    static Renderer instance;  // process-wide: the C interface has one implicit renderer',
        '    static unsigned int made = 0;',
        '    static const int fixed = 1;',
        '    static_assert(true);',
        '    return instance;',
        '}',
    ]), ['3: mutable-static-local']);
});

test('filelocal does not take a static member of a nested class for a static local', () => {
    assert.deepEqual(check('api/example/Example.h', [
        'class Example {',
        ' public:',
        '    struct Shape {',
        '        static Shape of(int part);',
        '        static int count;',
        '    };',
        '    void f() const {',
        '        static int calls = 0;',
        '    }',
        '};',
    ]), ['8: mutable-static-local']);
});

test('filelocal reads a const member function, a constructor list and an operator as functions', () => {
    assert.deepEqual(check(SOURCE, [
        'namespace {',
        'constexpr int FIRST = 1;',
        '};  // namespace',
        'int Example::size() const {',
        '    return 1;',
        '}',
        'Example::Example() : id_{ 1 }, name_("x") {',
        '}',
        'Example& Example::operator=(const Example& other) {',
        '    return *this;',
        '}',
        'auto Example::list() const -> std::vector<int> {',
        '    return {};',
        '}',
    ]), []);
});

test('filelocal reports an internal header included from outside its library', () => {
    const lines = ['#include <api/interchange/usd/internal/Reader.h>', '#include <api/interchange/usd/Usd.h>'];
    assert.deepEqual(check('api/interchange/usd/Usd.cxx', lines), []);
    assert.deepEqual(check('api/interchange/usd/tests/ReaderTest.cxx', lines), []);
    assert.deepEqual(check('vertical3d/src/scene/Import.cxx', lines), ['1: internal-include']);
    assert.deepEqual(check('api/interchange/Gltf.cxx', lines), ['1: internal-include']);
});

test('filelocal ignores comments, literals and preprocessor lines', () => {
    assert.deepEqual(check(SOURCE, [
        'void f() {',
        '}',
        '// int commented = 0;',
        '#define COUNT int x = 0;',
        'const char* const TEXT = "struct X { int y; };";',
    ]), ['5: outside-anonymous', '5: after-function']);
});
