/**
 * Tests for scripts/npm.ts, the check that npm brings only pinned development tools into the tree.
 *
 * Each test feeds the check a package.json, a lockfile, an .npmrc or a TypeScript file that breaks
 * one rule, beside one that keeps it. Run them with node --test "scripts/tests/*.test.ts".
 */

import assert from 'node:assert/strict';
import { spawnSync } from 'node:child_process';
import { join } from 'node:path';
import process from 'node:process';
import { test } from 'node:test';
import { fileURLToPath } from 'node:url';

import { isPackage, manifestFindings, packageImports, scriptsOff } from '../npm.ts';

const INTEGRITY = 'sha512-0000';

/** A lockfile that matches the given development dependencies, each fetched from the registry. */
function lockFor(dev: Record<string, string>): unknown {
    const packages: Record<string, unknown> = { '': { name: 'tooling', devDependencies: dev } };
    for (const [name, version] of Object.entries(dev)) {
        packages[`node_modules/${name}`] = {
            version, resolved: `https://registry.npmjs.org/${name}/-/${name}-${version}.tgz`, integrity: INTEGRITY, dev: true,
        };
    }
    return { lockfileVersion: 3, packages };
}

/** The findings for a manifest and a lockfile, as "rule: message" strings. */
function reported(manifest: unknown, lock: unknown): string[] {
    return manifestFindings(manifest, lock).map((f) => `${f.rule}: ${f.message}`);
}

test('npm accepts exact development dependencies with a matching lockfile', () => {
    const dev = { typescript: '7.0.2', '@types/node': '24.19.2' };
    assert.deepEqual(reported({ devDependencies: dev }, lockFor(dev)), []);
});

test('npm reports a runtime dependency, and an empty list of them is not one', () => {
    const dev = { typescript: '7.0.2' };
    assert.deepEqual(reported({ dependencies: {}, bundleDependencies: [], devDependencies: dev }, lockFor(dev)), []);
    const found = reported({ dependencies: { chalk: '5.0.0' }, peerDependencies: { a: '1.0.0' }, devDependencies: dev }, lockFor(dev));
    assert.deepEqual(found.map((f) => f.split(':')[0]), ['runtime-dependency', 'runtime-dependency']);
    assert.deepEqual(reported({ bundleDependencies: ['a'], devDependencies: dev }, lockFor(dev)).length, 1);
});

test('npm reports a range, a tag, a URL and a path, and accepts a prerelease', () => {
    for (const spec of ['^7.0.2', '~7.0.2', '>=7', '7.x', 'latest', 'github:microsoft/TypeScript', 'file:../ts', '*', '7.0']) {
        const dev = { typescript: spec };
        assert.deepEqual(reported({ devDependencies: dev }, lockFor(dev)).map((f) => f.split(':')[0]), ['inexact-version'], spec);
    }
    const dev = { typescript: '7.1.0-beta' };
    assert.deepEqual(reported({ devDependencies: dev }, lockFor(dev)), []);
});

test('npm reports a missing lockfile', () => {
    assert.deepEqual(reported({ devDependencies: { typescript: '7.0.2' } }, null), ['lockfile: there is no lockfile; run npm install and commit it']);
});

test('npm reports a lockfile that disagrees with package.json', () => {
    const lock = lockFor({ typescript: '7.0.1' });
    assert.deepEqual(reported({ devDependencies: { typescript: '7.0.2' } }, lock).map((f) => f.split(':')[0]), ['lockfile']);
    // a tool package.json no longer names, still in the lockfile
    assert.deepEqual(reported({ devDependencies: {} }, lock).map((f) => f.split(':')[0]), ['lockfile']);
});

test('npm reports a lockfile whose root names another version than package.json', () => {
    const lock = lockFor({ typescript: '7.0.2' }) as { packages: Record<string, { devDependencies?: Record<string, string> }> };
    const root = lock.packages[''];
    assert.ok(root !== undefined);
    root.devDependencies = { typescript: '7.0.1' };
    assert.deepEqual(reported({ devDependencies: { typescript: '7.0.2' } }, lock),
        ['lockfile: typescript is "7.0.2" in package.json and "7.0.1" in the lockfile']);
});

test('npm reports a locked tool installed at another version', () => {
    const lock = lockFor({ typescript: '7.0.2' }) as { packages: Record<string, { version: string }> };
    const entry = lock.packages['node_modules/typescript'];
    assert.ok(entry !== undefined);
    entry.version = '7.0.1';
    assert.deepEqual(reported({ devDependencies: { typescript: '7.0.2' } }, lock), ['lockfile: typescript 7.0.2 is locked at "7.0.1"']);
});

test('npm reports a locked package from outside the registry or without an integrity hash', () => {
    const dev = { typescript: '7.0.2' };
    const lock = lockFor(dev) as { packages: Record<string, unknown> };
    lock.packages['node_modules/from-git'] = { version: '1.0.0', resolved: 'git+ssh://git@example.com/a.git', integrity: INTEGRITY };
    lock.packages['node_modules/unhashed'] = { version: '1.0.0', resolved: 'https://registry.npmjs.org/unhashed/-/unhashed-1.0.0.tgz' };
    lock.packages['node_modules/elsewhere'] = { version: '1.0.0', resolved: 'https://registry.example.com/e.tgz', integrity: INTEGRITY };
    assert.deepEqual(reported({ devDependencies: dev }, lock), [
        'lockfile: node_modules/from-git does not come from https://registry.npmjs.org/ with an integrity hash',
        'lockfile: node_modules/unhashed does not come from https://registry.npmjs.org/ with an integrity hash',
        'lockfile: node_modules/elsewhere does not come from https://registry.npmjs.org/ with an integrity hash',
    ]);
});

test('npm reads ignore-scripts from .npmrc, and the last setting wins', () => {
    assert.equal(scriptsOff('ignore-scripts=true\n'), true);
    assert.equal(scriptsOff('# a comment\nignore-scripts = true  ; why\r\nsave-exact=true\n'), true);
    assert.equal(scriptsOff(''), false);
    assert.equal(scriptsOff('ignore-scripts=false\n'), false);
    assert.equal(scriptsOff('ignore-scripts=true\nignore-scripts=false\n'), false);
    assert.equal(scriptsOff('# ignore-scripts=true\n'), false);
});

test('npm counts a package specifier, and not a built-in or a file', () => {
    assert.equal(isPackage('typescript'), true);
    assert.equal(isPackage('@types/node'), true);
    // a built-in named without node: is reported too, because the tree always writes node:
    assert.equal(isPackage('fs'), true);
    assert.equal(isPackage('node:fs'), false);
    assert.equal(isPackage('./lexer.ts'), false);
    assert.equal(isPackage('../entry.ts'), false);
    assert.equal(isPackage('/abs/file.ts'), false);
});

test('npm reports each way of importing a package at run time', () => {
    const text = [
        "import ts from 'typescript';",
        'import {',
        '    a,',
        '    b,',
        "} from \"lodash\";",
        "export { c } from 'left-pad';",
        "export * from 'chalk';",
        "import 'side-effect';",
        "const later = await import('dynamic');",
        "import { type T } from 'kept-at-run-time';",
    ].join('\n');
    assert.deepEqual(packageImports(text).map((i) => `${i.line}: ${i.specifier}`), [
        '1: typescript', '2: lodash', '6: left-pad', '7: chalk', '8: side-effect', '9: dynamic', '10: kept-at-run-time',
    ]);
});

test('npm accepts a type-only import of a package, and imports of built-ins and files', () => {
    const text = [
        "import type { Node } from 'typescript';",
        "export type { Thing } from 'some-package';",
        "import { readFileSync } from 'node:fs';",
        "import { shouldRun } from './entry.ts';",
        "import * as lexer from '../lexer.ts';",
        "import process from 'node:process';",
    ].join('\n');
    assert.deepEqual(packageImports(text), []);
});

test('npm does not read an import inside a comment', () => {
    const text = [
        "// import ts from 'typescript';",
        '/**',
        " * import ts from 'typescript';",
        ' */',
        "const s = \"import x from 'y'\";",
    ].join('\n');
    assert.deepEqual(packageImports(text), []);
});

test('npm does not read an import inside a string, a template or a block comment', () => {
    const text = [
        "const line = \"const later = await import('dynamic');\";",
        'const template = `',
        "import ts from 'typescript';",
        "${'not code'} import('also-not')`;",
        "/* import('commented'); */ const real = await import('real');",
    ].join('\n');
    assert.deepEqual(packageImports(text).map((i) => `${i.line}: ${i.specifier}`), ['5: real']);
});

test('npm.ts runs when Node is started with it', () => {
    const root = fileURLToPath(new URL('../../', import.meta.url));
    const run = spawnSync(process.execPath, [join(root, 'scripts', 'npm.ts'), '--no-such-option'],
        { cwd: root, encoding: 'utf8' });
    assert.equal(run.status, 2, run.stderr);
    assert.match(run.stderr, /usage/);
});
