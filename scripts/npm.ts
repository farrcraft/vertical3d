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

import { execFileSync } from 'node:child_process';
import { existsSync, readFileSync } from 'node:fs';
import * as path from 'node:path';
import process from 'node:process';

import { shouldRun } from './entry.ts';

const ROOT = path.resolve(import.meta.dirname, '..');

/** The npm registry, which every package in the lockfile has to come from. */
const REGISTRY = 'https://registry.npmjs.org/';

/** The fields of package.json that name packages a consumer gets at run time. */
const RUNTIME_FIELDS = ['dependencies', 'optionalDependencies', 'peerDependencies', 'bundleDependencies', 'bundledDependencies'];

/** One exact version: major, minor and patch, with an optional prerelease and build. */
const EXACT = /^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?(?:\+[0-9A-Za-z.-]+)?$/;

export type Rule = 'runtime-dependency' | 'inexact-version' | 'lockfile' | 'install-scripts' | 'package-import';

/** One report, the file it is about, and the line when it has one. */
export interface Finding {
    file: string;
    line: number | null;
    rule: Rule;
    message: string;
}

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
            findings.push({ file: 'package.json', line: null, rule: 'runtime-dependency', message: `${field} names packages; only devDependencies are allowed` });
        }
    }
    const dev = record(pkg.devDependencies);
    for (const [name, spec] of Object.entries(dev)) {
        if (typeof spec !== 'string' || !EXACT.test(spec)) {
            findings.push({ file: 'package.json', line: null, rule: 'inexact-version', message: `${name} is ${JSON.stringify(spec)}, not one exact version` });
        }
    }
    if (lock === null) {
        findings.push({ file: 'package-lock.json', line: null, rule: 'lockfile', message: 'there is no lockfile; run npm install and commit it' });
        return findings;
    }
    const packages = record(record(lock).packages);
    const root = record(packages['']);
    const locked = record(root.devDependencies);
    for (const name of new Set([...Object.keys(dev), ...Object.keys(locked)])) {
        if (dev[name] !== locked[name]) {
            findings.push({ file: 'package-lock.json', line: null, rule: 'lockfile', message: `${name} is ${JSON.stringify(dev[name])} in package.json and ${JSON.stringify(locked[name])} in the lockfile` });
            continue;
        }
        const installed = record(packages[`node_modules/${name}`]).version;
        if (installed !== dev[name]) {
            findings.push({ file: 'package-lock.json', line: null, rule: 'lockfile', message: `${name} ${String(dev[name])} is locked at ${JSON.stringify(installed)}` });
        }
    }
    for (const field of RUNTIME_FIELDS) {
        if (root[field] !== undefined) {
            findings.push({ file: 'package-lock.json', line: null, rule: 'lockfile', message: `the root package lists ${field}` });
        }
    }
    for (const [where, value] of Object.entries(packages)) {
        if (where === '') {
            continue;
        }
        const entry = record(value);
        const resolved = entry.resolved;
        if (typeof resolved !== 'string' || !resolved.startsWith(REGISTRY) || typeof entry.integrity !== 'string') {
            findings.push({ file: 'package-lock.json', line: null, rule: 'lockfile', message: `${where} does not come from ${REGISTRY} with an integrity hash` });
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
 * The packages a TypeScript file imports at run time, each with the line its statement starts on.
 * A static import or export from a package counts, and so does an import for its side effects
 * and a dynamic import of a literal specifier.
 */
export function packageImports(text: string): Array<{ line: number; specifier: string }> {
    const found: Array<{ line: number; specifier: string }> = [];
    const lineOf = (index: number): number => text.slice(0, index).split('\n').length;
    // a statement that starts a line, whose clause before "from" may span lines
    const statement = /^[ \t]*(import|export)\b(?:\s+(type)\b)?([^;'"]*?)\bfrom\s*(['"])([^'"\n]+)\4/gm;
    for (const match of text.matchAll(statement)) {
        const specifier = match[5] ?? '';
        if (match[2] === undefined && isPackage(specifier)) {
            found.push({ line: lineOf(match.index), specifier });
        }
    }
    for (const match of text.matchAll(/^[ \t]*import\s*(['"])([^'"\n]+)\1/gm)) {
        const specifier = match[2] ?? '';
        if (isPackage(specifier)) {
            found.push({ line: lineOf(match.index), specifier });
        }
    }
    for (const match of text.matchAll(/\bimport\s*\(\s*(['"])([^'"\n]+)\1\s*\)/g)) {
        const specifier = match[2] ?? '';
        if (isPackage(specifier)) {
            found.push({ line: lineOf(match.index), specifier });
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
    if (process.argv.length > 2) {
        console.error('usage: node scripts/npm.ts');
        return 2;
    }
    const findings = manifestFindings(readJson('package.json'), readJson('package-lock.json'));
    const npmrc = path.join(ROOT, '.npmrc');
    if (!existsSync(npmrc) || !scriptsOff(readFileSync(npmrc, 'utf8'))) {
        findings.push({ file: '.npmrc', line: null, rule: 'install-scripts', message: 'install scripts are not turned off with ignore-scripts=true' });
    }
    const sources = execFileSync('git', ['ls-files', '-z', '--', '*.ts'], { cwd: ROOT, encoding: 'utf-8', maxBuffer: 64 * 1024 * 1024 })
        .split('\0')
        .filter((f) => f !== '');
    for (const file of sources) {
        for (const { line, specifier } of packageImports(readFileSync(path.join(ROOT, file), 'utf8'))) {
            findings.push({ file, line, rule: 'package-import', message: `imports ${specifier} at run time` });
        }
    }
    for (const { file, line, rule, message } of findings) {
        console.log(`${file}${line === null ? '' : `:${line}`}: ${rule}: ${message}`);
    }
    if (findings.length > 0) {
        console.log(`::error::${findings.length} findings above break "TypeScript tooling from npm" in docs/contributing/Dependencies.md.`);
        return 1;
    }
    return 0;
}

if (shouldRun(import.meta.url)) {
    process.exitCode = main();
}
