/**
 * Tests for the link rule in scripts/linkrule.ts and scripts/linkrule/.
 *
 * The first tests read includes and namespace uses from C++ text. The rest write a small tree to
 * a temporary directory, with a manifest, two api libraries and an app, an app library or a suite,
 * and check each rule on it. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { mkdirSync, mkdtempSync, rmSync, writeFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join } from 'node:path';
import { test } from 'node:test';

import { includesIn, usesIn, violations } from '../linkrule.ts';

const GRID = new Map([['grid', new Set(['v3dlib_grid'])]]);

test('linkrule counts a using-directive and not a namespace declaration', () => {
    const text = 'namespace v3d::grid {\n}\nusing namespace v3d::grid;\n';
    assert.deepEqual(usesIn(text, GRID), [[['v3dlib_grid'], 'v3d::grid']]);
});

test('linkrule does not count a name inside a comment or a literal', () => {
    const text = '// v3d::grid::Map\nconst char* s = "v3d::grid::Map";\n';
    assert.deepEqual(usesIn(text, GRID), []);
});

test('linkrule counts a name after a character literal holding a quote', () => {
    assert.deepEqual(usesIn("char q = '\"';\nv3d::grid::Map map;\n", GRID), [[['v3dlib_grid'], 'v3d::grid::Map']]);
});

test('linkrule counts a name after a CRLF-continued string, and not one inside it', () => {
    const text = 'const char* s = "a \\\r\nv3d::grid::Inside";\r\nv3d::grid::Outside o;\r\n';
    assert.deepEqual(usesIn(text, GRID), [[['v3dlib_grid'], 'v3d::grid::Outside']]);
});

test('linkrule reads includes outside comments and #if 0 blocks', () => {
    const text = [
        '#include "a.h"',
        '// #include "commented.h"',
        '#if 0',
        '#include "disabled.h"',
        '#else',
        '#include "kept.h"',
        '#endif',
        '  #  include <b.h>',
    ].join('\n');
    assert.deepEqual(includesIn(text), ['a.h', 'kept.h', 'b.h']);
});

/** The files every fixture tree has: the manifest, Boost's components and two api libraries. */
const BASE: Record<string, string> = {
    'cmake/v3dApiLibraries.cmake': 'set(V3D_API_core_PATH "core")\nset(V3D_API_gfx_PATH "gfx")\n',
    'cmake/v3dDependencies.cmake': 'find_package(Boost CONFIG REQUIRED COMPONENTS json unit_test_framework)\n',
    'api/core/CMakeLists.txt': 'v3d_add_api_library(core Core.cpp)\n',
    'api/core/Core.h': 'namespace v3d::core {\nclass Core {};\n};  // namespace v3d::core\n',
    'api/core/Core.cpp': '#include "Core.h"\n',
    'api/gfx/CMakeLists.txt': 'v3d_add_api_library(gfx Gfx.cpp)\ntarget_link_libraries(v3dlib_gfx PUBLIC v3dlib_core glm::glm)\n',
    'api/gfx/Gfx.h': '#include <api/core/Core.h>\n#include <glm/vec3.hpp>\nnamespace v3d::gfx {\nclass Gfx {};\n};  // namespace v3d::gfx\n',
    'api/gfx/Gfx.cpp': '#include "Gfx.h"\n',
};

/** The violations the link rule reports for the base tree with the given files added. */
function report(files: Record<string, string>): string[] {
    const root = mkdtempSync(join(tmpdir(), 'v3d-linkrule-'));
    try {
        const all = { ...BASE, ...files };
        for (const [file, text] of Object.entries(all)) {
            mkdirSync(join(root, dirname(file)), { recursive: true });
            writeFileSync(join(root, file), text);
        }
        const names = Object.keys(all).sort();
        const cmake = names.filter((f) => f.endsWith('CMakeLists.txt') || f.endsWith('.cmake'));
        return violations(root, cmake, names.filter((f) => f.startsWith('api/')));
    } finally {
        rmSync(root, { recursive: true, force: true });
    }
}

/** An app named app, built from main.cpp with the given text and linking the given items. */
function app(main: string, links: string, extra: Record<string, string> = {}): Record<string, string> {
    return {
        'app/CMakeLists.txt': `add_executable(app main.cpp)\ntarget_link_libraries(app PRIVATE ${links})\n`,
        'app/main.cpp': main,
        ...extra,
    };
}

const USES_GFX = '#include <api/gfx/Gfx.h>\nv3d::gfx::Gfx g;\n';

test('linkrule accepts an app that links what it includes', () => {
    assert.deepEqual(report(app(USES_GFX, 'v3dlib_gfx')), []);
});

test('linkrule reports an include of a library the app does not link', () => {
    assert.deepEqual(report(app(USES_GFX + '#include "api/core/Core.h"\n', 'v3dlib_gfx')),
        ['app/CMakeLists.txt: missing-link: app/main.cpp includes api/core/Core.h, and app does not link v3dlib_core']);
});

test('linkrule follows the app\'s own headers to the libraries they include', () => {
    const files = app('#include "local.h"\n', 'v3dlib_gfx', { 'app/local.h': USES_GFX });
    assert.deepEqual(report(files), []);
    const missing = app('#include "local.h"\n', 'v3dlib_core', { 'app/local.h': USES_GFX + 'v3d::core::Core c;\n#include <api/core/Core.h>\n' });
    assert.deepEqual(report(missing), ['app/CMakeLists.txt: missing-link: app/local.h includes api/gfx/Gfx.h, and app does not link v3dlib_gfx']);
});

test('linkrule reports a library the app links and none of its files includes', () => {
    assert.deepEqual(report(app(USES_GFX, 'v3dlib_gfx v3d::core')),
        ['app/CMakeLists.txt: extra-link: app links v3dlib_core, and none of its files includes its headers']);
});

test('linkrule reports a namespace used through another library\'s header', () => {
    assert.deepEqual(report(app(USES_GFX + 'v3d::core::Core c;\n', 'v3dlib_gfx')),
        ['app/main.cpp: namespace-use: names v3d::core::Core, and app includes no header of v3dlib_core']);
});

test('linkrule reports a package that a linked library already carries, or that no file includes', () => {
    assert.deepEqual(report(app(USES_GFX + '#include <glm/vec3.hpp>\n', 'v3dlib_gfx glm::glm')),
        ['app/CMakeLists.txt: package: app names glm::glm: glm::glm comes through a v3dlib_* target it links']);
    assert.deepEqual(report(app(USES_GFX, 'v3dlib_gfx Boost::json')),
        ['app/CMakeLists.txt: package: app names Boost::json: none of its files includes Boost::json']);
    assert.deepEqual(report(app(USES_GFX + '#include <boost/json.hpp>\n', 'v3dlib_gfx Boost::json')), []);
});

test('linkrule expands a package variable to the targets it names', () => {
    const png = USES_GFX + '#include <png.h>\n';
    assert.deepEqual(report(app(png, 'v3dlib_gfx ${PNG_LIBRARIES}')), []);
    assert.deepEqual(report(app(USES_GFX, 'v3dlib_gfx ${Boost_LIBRARIES}')), [
        'app/CMakeLists.txt: package: app names ${Boost_LIBRARIES}: none of its files includes Boost::json; none of its files includes Boost::unit_test_framework',
    ]);
});

test('linkrule reports a package header that nothing the app links carries', () => {
    assert.deepEqual(report(app(USES_GFX + '#include <png.h>\n', 'v3dlib_gfx')),
        ['app/CMakeLists.txt: package-unlinked: app/main.cpp includes png.h, and nothing app links carries PNG::PNG']);
    // a package a linked library carries reaches the app
    assert.deepEqual(report(app(USES_GFX + '#include <glm/vec3.hpp>\n', 'v3dlib_gfx')), []);
});

test('linkrule gives a suite the test framework, and refuses it named again', () => {
    const suite = (links: string) => ({
        'app/tests/CMakeLists.txt': `v3d_add_test(app_suite Test.cpp)\ntarget_link_libraries(v3dtest_app_suite PRIVATE ${links})\n`,
        'app/tests/Test.cpp': USES_GFX + '#include <boost/test/unit_test.hpp>\n',
    });
    assert.deepEqual(report(suite('v3dlib_gfx')), []);
    assert.deepEqual(report(suite('v3dlib_gfx Boost::unit_test_framework')),
        ['app/tests/CMakeLists.txt: package: v3dtest_app_suite names Boost::unit_test_framework: v3d_add_test already links Boost::unit_test_framework']);
});

test('linkrule reads a header under an api tests directory as the suite\'s own', () => {
    const files = {
        'api/core/tests/CMakeLists.txt': 'v3d_add_test(core Test.cpp)\ntarget_link_libraries(v3dtest_core PRIVATE v3dlib_gfx)\n',
        'api/core/tests/Test.cpp': '#include "api/core/tests/Helper.h"\n',
        'api/core/tests/Helper.h': USES_GFX,
    };
    // the helper is followed, so the suite includes gfx through it, and does not include core
    assert.deepEqual(report(files), []);
});

test('linkrule does not pass a library\'s PRIVATE links on to what links it', () => {
    const files = {
        'api/core/CMakeLists.txt': 'v3d_add_api_library(core Core.cpp)\ntarget_link_libraries(v3dlib_core PRIVATE PNG::PNG)\n',
        ...app(USES_GFX + '#include <png.h>\n', 'v3dlib_gfx'),
    };
    assert.deepEqual(report(files),
        ['app/CMakeLists.txt: package-unlinked: app/main.cpp includes png.h, and nothing app links carries PNG::PNG']);
});

/**
 * An app library in lib/, with a header and a source, linking gfx as given. Its name starts
 * v3dlib_, as every app library's does.
 */
function applib(header: string, source: string, link: string): Record<string, string> {
    return {
        'lib/CMakeLists.txt': `add_library(v3dlib_applib Lib.cpp)\ntarget_link_libraries(v3dlib_applib ${link})\n`,
        'lib/Lib.h': header,
        'lib/Lib.cpp': source,
    };
}

test('linkrule holds an app library\'s PUBLIC links to what its headers include', () => {
    assert.deepEqual(report(applib(USES_GFX, '#include "Lib.h"\n', 'PUBLIC v3dlib_gfx')), []);
    assert.deepEqual(report(applib(USES_GFX, '#include "Lib.h"\n', 'PRIVATE v3dlib_gfx')),
        ['lib/CMakeLists.txt: visibility: lib/Lib.h includes api/gfx/Gfx.h, and v3dlib_applib links v3dlib_gfx PRIVATE']);
    assert.deepEqual(report(applib('', USES_GFX, 'PUBLIC v3dlib_gfx')),
        ['lib/CMakeLists.txt: visibility: v3dlib_applib links v3dlib_gfx PUBLIC, and none of its headers includes it']);
    assert.deepEqual(report(applib('', USES_GFX, 'PRIVATE v3dlib_gfx')), []);
});

test('linkrule reads an app library\'s internal header as private to it', () => {
    const files = {
        ...applib('', '#include "internal/Detail.h"\n', 'PRIVATE v3dlib_gfx'),
        'lib/internal/Detail.h': USES_GFX,
    };
    assert.deepEqual(report(files), []);
});

test('linkrule reads an app that uses an app library through the library\'s headers', () => {
    const files = {
        ...applib(USES_GFX, '#include "Lib.h"\n', 'PUBLIC v3dlib_gfx'),
        ...app('#include "lib/Lib.h"\n', 'v3dlib_applib'),
    };
    assert.deepEqual(report(files), []);
});

test('linkrule does not count an include inside a comment or an #if 0 block', () => {
    const main = USES_GFX + '// #include <api/core/Core.h>\n#if 0\n#include <png.h>\n#endif\n';
    assert.deepEqual(report(app(main, 'v3dlib_gfx')), []);
});
