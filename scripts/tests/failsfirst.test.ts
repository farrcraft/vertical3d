/**
 * Tests for the fails-first harness in scripts/failsfirst.ts and scripts/failsfirst/.
 *
 * Each test checks one step the harness takes without building anything: finding the cases a
 * source declares, reading a CMake list or a failed build, and turning how a run ended into a
 * result. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { mkdirSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { test } from 'node:test';

import { parseOptions } from '../failsfirst.ts';
import { dropNote, failedSources } from '../failsfirst/build.ts';
import { casesIn, inBase, namedCommit } from '../failsfirst/cases.ts';
import { dropArgument, dropCall, dropSource, missingLinks, missingSources, sourceOf, unknownCommands } from '../failsfirst/cmake.ts';
import { classify, describeExit, type Outcome } from '../failsfirst/run.ts';
import { type Case, FAILS, NOT_RUN, PASSES, PASSES_COVERS, PASSES_STATED, SKIPPED } from '../failsfirst/types.ts';
import { ROOT } from '../lib/tree.ts';

test('failsfirst finds a case after a CRLF-continued string and after a raw string', () => {
    const text = [
        'BOOST_AUTO_TEST_SUITE(suite)',
        'const char* a = "one \\',
        'two /* not a comment";',
        'BOOST_AUTO_TEST_CASE(after_continued) {}',
        'const char* b = R"(" /* )";',
        'BOOST_AUTO_TEST_CASE(after_raw) {}',
        '/* a real comment */',
        'BOOST_AUTO_TEST_SUITE_END()',
        '',
    ].join('\r\n');
    assert.deepEqual(casesIn(text).map((c) => c.path), ['suite/after_continued', 'suite/after_raw']);
});

test('failsfirst names each kind of case, inside nested suites, and a data case by its second argument', () => {
    const text = [
        'BOOST_AUTO_TEST_SUITE(outer)',
        'BOOST_FIXTURE_TEST_SUITE(inner, Fixture)',
        'BOOST_FIXTURE_TEST_CASE(fixtured, Fixture) {}',
        'BOOST_DATA_TEST_CASE(data, values, value) {}',
        'BOOST_DATA_TEST_CASE_F(Fixture, data_fixture, values, value) {}',
        'BOOST_AUTO_TEST_CASE_TEMPLATE(templated, T, Types) {}',
        'BOOST_AUTO_TEST_SUITE_END()',
        'BOOST_AUTO_TEST_CASE(in_outer) {}',
        'BOOST_AUTO_TEST_SUITE_END()',
        'BOOST_AUTO_TEST_CASE(at_top) {}',
    ].join('\n');
    assert.deepEqual(casesIn(text).map((c) => c.path), [
        'outer/inner/fixtured', 'outer/inner/data', 'outer/inner/data_fixture', 'outer/inner/templated',
        'outer/in_outer', 'at_top',
    ]);
});

test('failsfirst reads a Passes before the change: line, wrapped to the end of its paragraph', () => {
    const text = [
        '// Draws twice.',
        '//',
        '// Passes before the change: the second draw',
        '// only exists with the change.',
        '//',
        '// A later paragraph.',
        'BOOST_AUTO_TEST_CASE(stated) {}',
    ].join('\n');
    const [found] = casesIn(text);
    assert.equal(found?.stated, 'the second draw only exists with the change.');
    assert.equal(found?.covers, null);
});

test('failsfirst reads a Covers the base: line, wrapped onto the next line', () => {
    const text = [
        '/**',
        ' * Pans each way.',
        ' *',
        ' * Covers the base: 48412a1c merged the controller untested. Checked by swapping two',
        ' * pan directions, which fails it.',
        ' **/',
        'BOOST_AUTO_TEST_CASE(pans) {}',
        'BOOST_AUTO_TEST_CASE(plain) {}',
        '',
    ].join('\n');
    const found = casesIn(text);
    assert.equal(found[0]?.covers,
        '48412a1c merged the controller untested. Checked by swapping two pan directions, which fails it.');
    assert.equal(found[0]?.stated, null);
    assert.equal(found[1]?.covers, null);
    assert.equal(namedCommit(found[0]?.covers ?? ''), '48412a1c');
    assert.equal(namedCommit('no commit named here'), null);
    // a word of six hex letters is too short to name a commit
    assert.equal(namedCommit('the decade fix in abcdef'), null);
});

test('failsfirst accepts a covered commit only when the base contains it', () => {
    // two commits on main: 957a7434 is the second parent of the merge 48412a1c
    assert.equal(inBase('957a7434 brought it', '48412a1c'), '957a74343d34043c61315af330384508b6db4c97');
    assert.equal(inBase('48412a1c brought it', '957a7434'), null);
    assert.equal(inBase('deadbeefdeadbeef brought it', '48412a1c'), null);
    assert.equal(inBase('no commit at all', '48412a1c'), null);
});

test('failsfirst skips a case inside a comment', () => {
    const text = '// BOOST_AUTO_TEST_CASE(commented) {}\n/* BOOST_AUTO_TEST_CASE(blocked) {} */\nBOOST_AUTO_TEST_CASE(real) {}\n';
    assert.deepEqual(casesIn(text).map((c) => c.path), ['real']);
});

test('failsfirst reads an unknown command from a failed configure', () => {
    const output = [
        '-- vertical3d api: asset',
        'CMake Error at vertical3d/tests/CMakeLists.txt:75 (v3d_copy_usd_plugins):',
        '  Unknown CMake command "v3d_copy_usd_plugins".',
        '',
        'CMake Error at api/x/tests/CMakeLists.txt:3 (v3d_add_test):',
        '  Cannot find source file:',
        '',
    ].join('\r\n');
    assert.deepEqual(unknownCommands(output),
        [{ file: 'vertical3d/tests/CMakeLists.txt', line: 75, command: 'v3d_copy_usd_plugins' }]);
});

test('failsfirst reads each source a failed configure cannot find, as the list spelled it', () => {
    const output = [
        'CMake Error at api/x/tests/CMakeLists.txt:3 (add_executable):',
        '  Cannot find source file:',
        '',
        '    ../src/Added.cxx',
        '',
        'CMake Error at api/y/tests/CMakeLists.txt:3 (add_executable):',
        '  Cannot find source file:',
        '    C:/work/api/y/New.cpp',
    ].join('\r\n');
    assert.deepEqual(missingSources(output), ['../src/Added.cxx', 'C:/work/api/y/New.cpp']);
    assert.deepEqual(missingSources('-- Configuring done'), []);
});

test('failsfirst drops a call that spans lines and holds parentheses', () => {
    const text = 'a(x)\nv3d_helper(t\n    "$<A:(b)>" ")"\n    (c))\nb(y)\n';
    assert.equal(dropCall(text, 2, 'v3d_helper'), 'a(x)\n\nb(y)\n');
    assert.equal(dropCall(text, 2, 'V3D_HELPER'), 'a(x)\n\nb(y)\n');
    assert.equal(dropCall(text, 1, 'v3d_helper'), null);
    assert.equal(dropCall(text, 9, 'v3d_helper'), null);
});

test('failsfirst reads each link to a target a failed configure cannot find', () => {
    const output = [
        'CMake Error at api/font/tests/CMakeLists.txt:20 (target_link_libraries):',
        '  Target "v3dtest_font" links to:',
        '',
        '    ICU::uc',
        '',
        '  but the target was not found.  Possible reasons include:',
        '',
        'CMake Error at api/font/tests/CMakeLists.txt:20 (target_link_libraries):',
        '  Target "v3dtest_font" links to:',
        '',
        '    thorvg::thorvg',
        '',
        '  but the target was not found.  Possible reasons include:',
        '',
        'CMake Error at api/x/tests/CMakeLists.txt:3 (v3d_helper):',
        '  Unknown CMake command "v3d_helper".',
    ].join('\r\n');
    assert.deepEqual(missingLinks(output), [
        { file: 'api/font/tests/CMakeLists.txt', line: 20, target: 'ICU::uc' },
        { file: 'api/font/tests/CMakeLists.txt', line: 20, target: 'thorvg::thorvg' },
    ]);
    assert.deepEqual(missingLinks('-- Configuring done'), []);
});

test('failsfirst drops one item from a call, and only a whole item', () => {
    const text = 'a(x)\ntarget_link_libraries(t PRIVATE ICU::uc2 ICU::uc\n    ICU::data)\nb(ICU::uc)\n';
    assert.equal(dropArgument(text, 2, 'target_link_libraries', 'ICU::uc'),
        'a(x)\ntarget_link_libraries(t PRIVATE ICU::uc2\n    ICU::data)\nb(ICU::uc)\n');
    assert.equal(dropArgument(text, 2, 'target_link_libraries', 'ICU::data'),
        'a(x)\ntarget_link_libraries(t PRIVATE ICU::uc2 ICU::uc)\nb(ICU::uc)\n');
    assert.equal(dropArgument(text, 2, 'target_link_libraries', 'ICU::i18n'), null);
    assert.equal(dropArgument(text, 1, 'target_link_libraries', 'ICU::uc'), null);
});

test('failsfirst resolves a list entry against its directory, and gives up on another variable', () => {
    assert.equal(sourceOf('api/ui/tests', 'ScreenTest.cpp'), 'api/ui/tests/ScreenTest.cpp');
    assert.equal(sourceOf('api/ui/tests', '${CMAKE_CURRENT_SOURCE_DIR}/ScreenTest.cpp'), 'api/ui/tests/ScreenTest.cpp');
    assert.equal(sourceOf('vertical3d/tests', '../src/app/Thing.cxx'), 'vertical3d/src/app/Thing.cxx');
    assert.equal(sourceOf('api/ui/tests', '${OTHER}/ScreenTest.cpp'), null);
});

test('failsfirst drops a source from its suite\'s list, quoted or not, and leaves the rest', () => {
    const worktree = mkdtempSync(join(tmpdir(), 'v3d-failsfirst-'));
    try {
        mkdirSync(join(worktree, 'api', 'ui', 'tests'), { recursive: true });
        const file = join(worktree, 'api', 'ui', 'tests', 'CMakeLists.txt');
        writeFileSync(file, [
            '# A.cpp is named in this comment',
            'v3d_add_test(other A.cpp)',
            'v3d_add_test(ui',
            '    # B.cpp is named in a comment inside the list',
            '    ${CMAKE_CURRENT_SOURCE_DIR}/A.cpp',
            '    "B.cpp"',
            '    C.cpp)',
            '',
        ].join('\n'));
        assert.equal(dropSource(worktree, 'api/ui/tests', 'ui', 'api/ui/tests/B.cpp'), true);
        assert.equal(dropSource(worktree, 'api/ui/tests', 'ui', 'api/ui/tests/A.cpp'), true);
        assert.equal(dropSource(worktree, 'api/ui/tests', 'ui', 'api/ui/tests/Missing.cpp'), false);
        assert.equal(dropSource(worktree, 'api/ui/tests', 'nosuite', 'api/ui/tests/C.cpp'), false);
        assert.equal(dropSource(worktree, 'api/none/tests', 'ui', 'api/none/tests/C.cpp'), false);
        assert.equal(readFileSync(file, 'utf8'), [
            '# A.cpp is named in this comment',
            'v3d_add_test(other A.cpp)',
            'v3d_add_test(ui',
            '    # B.cpp is named in a comment inside the list',
            '    ',
            '    ',
            '    C.cpp)',
            '',
        ].join('\n'));
    } finally {
        rmSync(worktree, { recursive: true, force: true });
    }
});

const SOURCES = ['api/ui/tests/ScreenTest.cpp', 'api/ui/tests/ThemeTest.cpp', 'api/ui/src/Helper.cxx'];

test('failsfirst names the suite sources that failed to compile, including one outside tests/', () => {
    const output = [
        '[1/3] Building CXX object api/ui/tests/CMakeFiles/v3dtest_ui.dir/ThemeTest.cpp.obj',
        'FAILED: [code=2] api/ui/tests/CMakeFiles/v3dtest_ui.dir/ScreenTest.cpp.obj',
        'FAILED: api/ui/tests/CMakeFiles/v3dtest_ui.dir/__/src/Helper.cxx.obj',
    ].join('\n');
    assert.deepEqual(failedSources(output, 'api/ui/tests', 'ui', SOURCES), ['api/ui/tests/ScreenTest.cpp', 'api/ui/src/Helper.cxx']);
    assert.deepEqual(failedSources('ninja: no work to do.', 'api/ui/tests', 'ui', SOURCES), []);
});

test('failsfirst gives up on a failure that is not one of the suite\'s own objects', () => {
    const library = 'FAILED: api/ui/CMakeFiles/v3dlib_ui.dir/Screen.cpp.obj\n';
    assert.equal(failedSources(library, 'api/ui/tests', 'ui', SOURCES), null);
    const other = 'FAILED: api/font/tests/CMakeFiles/v3dtest_font.dir/FontTest.cpp.obj\n';
    assert.equal(failedSources(other, 'api/ui/tests', 'ui', SOURCES), null);
});

test('failsfirst names the object an unresolved symbol comes from, by its source', () => {
    const output = [
        'FAILED: api/ui/tests/v3dtest_ui.exe',
        'ScreenTest.cpp.obj : error LNK2019: unresolved external symbol "void __cdecl added(void)"',
        '  api\\ui\\tests\\CMakeFiles\\v3dtest_ui.dir\\ScreenTest.cpp.obj : error LNK2001: unresolved external symbol x',
        'api/ui/tests/v3dtest_ui.exe : fatal error LNK1120: 2 unresolved externals',
    ].join('\r\n');
    assert.deepEqual(failedSources(output, 'api/ui/tests', 'ui', SOURCES), ['api/ui/tests/ScreenTest.cpp']);
});

test('failsfirst gives up on a link error that no one source answers for', () => {
    const exe = 'FAILED: api/ui/tests/v3dtest_ui.exe\n';
    // a library the link cannot open, even beside a symbol a test object leaves unresolved
    assert.equal(failedSources(exe + 'LINK : fatal error LNK1104: cannot open file v3dlib_new.lib\n', 'api/ui/tests', 'ui', SOURCES), null);
    assert.equal(failedSources(exe + 'ScreenTest.cpp.obj : error LNK2019: unresolved external symbol z\n' +
        'LINK : fatal error LNK1104: cannot open file v3dlib_new.lib\n', 'api/ui/tests', 'ui', SOURCES), null);
    // an unresolved symbol in an object no source of the suite makes
    assert.equal(failedSources(exe + 'Screen.cpp.obj : error LNK2019: unresolved external symbol y\n', 'api/ui/tests', 'ui', SOURCES), null);
    // an object two sources of the same name could have made
    const twins = [...SOURCES, 'api/ui/tests/more/ScreenTest.cpp'];
    assert.equal(failedSources(exe + 'ScreenTest.cpp.obj : error LNK2019: unresolved external symbol z\n', 'api/ui/tests', 'ui', twins), null);
    // a failed link that names no object
    assert.equal(failedSources(exe, 'api/ui/tests', 'ui', SOURCES), null);
});

test('failsfirst says why a suite that lost sources may not build', () => {
    assert.equal(dropNote([{ source: 'a.cxx', added: true }, { source: 'b.cxx', added: true }]),
        'the suite needs a.cxx, b.cxx, which the change added');
    assert.equal(dropNote([{ source: 'a.cxx', added: true }, { source: 'c.cxx', added: false }]),
        'the suite needs a.cxx, which the change added; the suite lists c.cxx, which the worktree does not have');
    assert.equal(dropNote([]), '');
});

/** A case with nothing said about passing without the change, in the given suite. */
function plainCase(suite = 'ui', extra: Partial<Case> = {}): Case {
    return { path: 'suite/case', suite, file: 'f.cpp', stated: null, claim: null, covers: null, result: null, note: '', ...extra };
}

/** The result and note a run's ending gives a case. */
function classified(done: Partial<Outcome>, c: Case = plainCase()): string {
    classify(c, { error: undefined, status: 1, signal: null, output: '', ...done }, 900);
    return c.note === '' ? String(c.result) : `${String(c.result)} (${c.note})`;
}

test('failsfirst counts only a failure Boost.Test reports as failing', () => {
    assert.equal(classified({ output: '*** 1 failure is detected in the test module "ui"' }), FAILS);
    assert.equal(classified({ output: '*** 3 failures are detected in the test module "ui"' }), FAILS);
    assert.equal(classified({ output: 'f.cpp(12): error: in "suite/case": check x == y has failed' }), FAILS);
    assert.equal(classified({ output: 'unknown location(0): fatal error: in "suite/case": memory access violation\nfailure occurred' }), FAILS);
    assert.equal(classified({ status: 3221225781, output: '' }), `${NOT_RUN} (exited with 0xC0000135 and reported no failed check)`);
    assert.equal(classified({ status: 1, output: 'Test setup error: no test cases matching filter or all test cases were disabled' }),
        `${NOT_RUN} (the binary has no such case)`);
    assert.equal(classified({ status: 200, output: 'Test setup error: test tree is empty' }), `${NOT_RUN} (the binary has no such case)`);
    // a binary that printed something, and died before Boost.Test reported, has not shown a failure
    assert.equal(classified({ status: 1, output: 'Running 1 test case...\nloading the device\n' }),
        `${NOT_RUN} (exited with 1 and reported no failed check)`);
});

test('failsfirst reports a passing case as weak unless its doc comment explains it', () => {
    assert.equal(classified({ status: 0 }), PASSES);
    assert.equal(classified({ status: 0 }, plainCase('ui', { stated: 'why' })), PASSES_STATED);
    assert.equal(classified({ status: 0 }, plainCase('ui', { claim: 'abc1234 brought it', covers: 'abc1234def' })), PASSES_COVERS);
    assert.equal(classified({ status: 0 }, plainCase('ui', { claim: 'no commit', covers: null })),
        `${PASSES} (its Covers the base: line names no commit the base contains)`);
});

test('failsfirst skips a case only in a suite that may find no device', () => {
    assert.equal(classified({ status: 77 }, plainCase('render_device')), SKIPPED);
    assert.equal(classified({ status: 77 }, plainCase('render_window')), SKIPPED);
    assert.equal(classified({ status: 77 }, plainCase('ui')), `${NOT_RUN} (exited with 77 and reported no failed check)`);
});

test('failsfirst reports a run that timed out or could not start as not run', () => {
    const timedOut = Object.assign(new Error('spawnSync ETIMEDOUT'), { code: 'ETIMEDOUT' });
    assert.equal(classified({ error: timedOut, status: null }), `${NOT_RUN} (timed out after 900s)`);
    assert.equal(classified({ error: new Error('spawnSync x ENOENT'), status: null }), `${NOT_RUN} (could not start: spawnSync x ENOENT)`);
});

test('failsfirst writes an exit status as Windows reports it', () => {
    assert.equal(describeExit(1, null), '1');
    assert.equal(describeExit(0xC0000005, null), '0xC0000005');
    assert.equal(describeExit(-1073741819, null), '0xC0000005');
    assert.equal(describeExit(null, 'SIGTERM'), 'signal SIGTERM');
    assert.equal(describeExit(null, null), 'no status');
});

test('failsfirst reads its options, and refuses a timeout that is not a whole number', () => {
    const defaults = parseOptions([]);
    assert.deepEqual(defaults, {
        base: null, commit: null, list: false, keep: false, scratch: null, timeout: 900, build: join(ROOT, 'out', 'build', 'x64-Debug'),
    });
    assert.equal(parseOptions(['--help']), null);
    assert.equal(parseOptions(['-h']), null);
    assert.deepEqual(parseOptions(['--base', 'main', '--list', '--timeout', '60', '--build', 'out/build/ci']), {
        base: 'main', commit: null, list: true, keep: false, scratch: null, timeout: 60, build: join(ROOT, 'out', 'build', 'ci'),
    });
    for (const timeout of ['0', '1.5', '12abc', 'x', '']) {
        assert.throws(() => parseOptions(['--timeout', timeout]), /whole number|usage/, timeout);
    }
    // a negative number reads as an option, which the harness does not take
    assert.throws(() => parseOptions(['--timeout', '-5']), /usage/);
    assert.throws(() => parseOptions(['--base', 'main', '--commit', 'abc', '--nope']), /usage/);
});
