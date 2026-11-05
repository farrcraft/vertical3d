/**
 * Check that npm brings only pinned development tools into the tree, as
 * docs/contributing/Dependencies.md sets out under "TypeScript tooling from npm".
 *
 * The script reads package.json, package-lock.json, .npmrc and every tracked .ts file, and
 * reports:
 *
 *   runtime-dependency  package.json lists dependencies, optional, peer or bundled dependencies.
 *                       Only development dependencies are allowed.
 *   inexact-version     a development dependency whose version is not one exact version, such
 *                       as a range, a tag, a URL or a path.
 *   lockfile            package-lock.json is missing, names a version package.json does not, or
 *                       holds a package that was not fetched from the npm registry with an
 *                       integrity hash.
 *   install-scripts     .npmrc does not turn off install scripts with ignore-scripts=true.
 *   package-import      a TypeScript file imports a package at run time. A specifier that starts
 *                       with node:, ./, ../ or / is not a package. An import type or export type
 *                       statement is erased before the file runs, so it may name one.
 *
 * Imports are found by reading statements that start a line, so an import inside a comment or a
 * string is not counted. A statement that starts after other code on its line is not read.
 *
 * Usage: node scripts/npm.ts
 */

import { existsSync, readFileSync } from 'node:fs';
import * as path from 'node:path';

import { type Finding, options, report, runGate } from './lib/cli.ts';
import { tracked } from './lib/git.ts';
import { codeOnly } from './lib/script.ts';
import { ROOT } from './lib/tree.ts';

/** The npm registry, which every package in the lockfile has to come from. */
const REGISTRY = 'https://registry.npmjs.org/';

/** The fields of package.json that name packages a consumer gets at run time. */
const RUNTIME_FIELDS = ['dependencies', 'optionalDependencies', 'peerDependencies', 'bundleDependencies', 'bundledDependencies'];

/** One exact version: major, minor and patch, with an optional prerelease and build. */
const EXACT = /^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/;

/** An object read from JSON, or an empty one for anything else. */
function record(value: unknown): Record<string, unknown> {
    return typeof value === 'object' && value !== null && !Array.isArray(value) ? value as Record<string, unknown> : {};
}

/** The findings for package.json and its lockfile, each parsed. A missing lockfile is null. */
export function manifestFindings(manifest: unknown, lock: unknown): Finding[] {
    const findings: Finding[] = [];
    const pkg = record(manifest);
    for (const field of RUNTIME_FIELDS) {
        const value = pkg[field];
        if (value !== undefined && (Array.isArray(value) ? value.length > 0 : Object.keys(record(value)).length > 0)) {
            findings.push({ path: 'package.json', line: null, rule: 'runtime-dependency', message: `${field} names packages; only devDependencies are allowed` });
        }
    }
    const dev = record(pkg.devDependencies);
    for (const [name, spec] of Object.entries(dev)) {
        if (typeof spec !== 'string' || !EXACT.test(spec)) {
            findings.push({ path: 'package.json', line: null, rule: 'inexact-version', message: `${name} is ${JSON.stringify(spec)}, not one exact version` });
        }
    }
    if (lock === null) {
        findings.push({ path: 'package-lock.json', line: null, rule: 'lockfile', message: 'there is no lockfile; run npm install and commit it' });
        return findings;
    }
    const packages = record(record(lock).packages);
    const root = record(packages['']);
    const locked = record(root.devDependencies);
    for (const name of new Set([...Object.keys(dev), ...Object.keys(locked)])) {
        if (dev[name] !== locked[name]) {
            findings.push({ path: 'package-lock.json', line: null, rule: 'lockfile', message: `${name} is ${JSON.stringify(dev[name])} in package.json and ${JSON.stringify(locked[name])} in the lockfile` });
            continue;
        }
        const installed = record(packages[`node_modules/${name}`]).version;
        if (installed !== dev[name]) {
            findings.push({ path: 'package-lock.json', line: null, rule: 'lockfile', message: `${name} ${String(dev[name])} is locked at ${JSON.stringify(installed)}` });
        }
    }
    for (const field of RUNTIME_FIELDS) {
        if (root[field] !== undefined) {
            findings.push({ path: 'package-lock.json', line: null, rule: 'lockfile', message: `the root package lists ${field}` });
        }
    }
    for (const [where, value] of Object.entries(packages)) {
        if (where === '') {
            continue;
        }
        const entry = record(value);
        const resolved = entry.resolved;
        if (typeof resolved !== 'string' || !resolved.startsWith(REGISTRY) || typeof entry.integrity !== 'string') {
            findings.push({ path: 'package-lock.json', line: null, rule: 'lockfile', message: `${where} does not come from ${REGISTRY} with an integrity hash` });
        }
    }
    return findings;
}

/** Whether .npmrc turns off install scripts. */
export function scriptsOff(npmrc: string): boolean {
    let off = false;
    for (const raw of npmrc.split(/\r?\n/)) {
        const line = raw.replace(/[#;].*$/, '').trim();
        const match = /^ignore-scripts\s*=\s*(\S+)$/.exec(line);
        if (match) {
            off = match[1] === 'true';
        }
    }
    return off;
}

/** Whether an import specifier names a package rather than a built-in module or a file. */
export function isPackage(specifier: string): boolean {
    return !/^(node:|\.\.?\/|\/)/.test(specifier);
}

/**
 * Each statement that imports at run time: a static import or export from a specifier, an import
 * for its side effects, and a dynamic import of a literal specifier. The patterns are matched on
 * the code alone, where a specifier's quotes stay and its text is blanked, and the last group of
 * each is the specifier's place. A static import starts a line, and its clause may span lines.
 */
const IMPORTS: RegExp[] = [
    /^[ \t]*(?:import|export)\b(?!\s+type\b)[^;'"]*?\bfrom\s*['"]( *)['"]/dgm,
    /^[ \t]*import\s*['"]( *)['"]/dgm,
    /\bimport\s*\(\s*['"]( *)['"]\s*\)/dg,
];

/**
 * The packages a TypeScript file imports at run time, each with the line its statement starts on.
 * An import or export inside a comment or a literal is not one.
 */
export function packageImports(text: string): Array<{ line: number; specifier: string }> {
    const code = codeOnly(text);
    const found: Array<{ line: number; specifier: string }> = [];
    for (const pattern of IMPORTS) {
        for (const match of code.matchAll(pattern)) {
            const [start, end] = match.indices?.[1] ?? [0, 0];
            const specifier = text.slice(start, end);
            if (specifier !== '' && isPackage(specifier)) {
                found.push({ line: text.slice(0, match.index).split('\n').length, specifier });
            }
        }
    }
    return found.sort((a, b) => a.line - b.line);
}

/** A JSON file under the root, parsed, or null when it does not exist. */
function readJson(name: string): unknown {
    const file = path.join(ROOT, name);
    return existsSync(file) ? JSON.parse(readFileSync(file, 'utf8')) : null;
}

function main(): number {
    // the check takes no arguments, so an option meant for it is refused rather than ignored
    options('usage: node scripts/npm.ts', {});
    const findings = manifestFindings(readJson('package.json'), readJson('package-lock.json'));
    const npmrc = path.join(ROOT, '.npmrc');
    if (!existsSync(npmrc) || !scriptsOff(readFileSync(npmrc, 'utf8'))) {
        findings.push({ path: '.npmrc', line: null, rule: 'install-scripts', message: 'install scripts are not turned off with ignore-scripts=true' });
    }
    for (const file of tracked('*.ts')) {
        for (const { line, specifier } of packageImports(readFileSync(path.join(ROOT, file), 'utf8'))) {
            findings.push({ path: file, line, rule: 'package-import', message: `imports ${specifier} at run time` });
        }
    }
    return report(findings, 'breaks "TypeScript tooling from npm" in docs/contributing/Dependencies.md.');
}

runGate(import.meta.url, main);
