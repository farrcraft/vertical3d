/**
 * Finding the Boost.Test cases a change adds, and what their doc comments say about passing
 * without it.
 */

import { blankComments } from '../lib/lexer.ts';
import { ROOT, isTestPath } from '../lib/tree.ts';
import { suiteMap } from './cmake.ts';
import { git, show } from './repo.ts';
import type { Case, Change, Found, SuiteEntry } from './types.ts';

// The tokens that open and close a Boost.Test suite and that declare a case. A data case with
// a fixture names the case in its second argument, and every other macro in its first.
const TOKEN = new RegExp(
    '\\b(BOOST_AUTO_TEST_SUITE_END|BOOST_AUTO_TEST_SUITE|BOOST_FIXTURE_TEST_SUITE' +
    '|BOOST_AUTO_TEST_CASE_TEMPLATE|BOOST_FIXTURE_TEST_CASE_TEMPLATE' +
    '|BOOST_DATA_TEST_CASE_F|BOOST_AUTO_TEST_CASE|BOOST_FIXTURE_TEST_CASE|BOOST_DATA_TEST_CASE)' +
    '\\s*\\(([^)]*)', 'g');
const NAME_ARGUMENT: Record<string, number> = {
    BOOST_AUTO_TEST_SUITE: 0,
    BOOST_FIXTURE_TEST_SUITE: 0,
    BOOST_AUTO_TEST_CASE: 0,
    BOOST_FIXTURE_TEST_CASE: 0,
    BOOST_DATA_TEST_CASE: 0,
    BOOST_AUTO_TEST_CASE_TEMPLATE: 0,
    BOOST_FIXTURE_TEST_CASE_TEMPLATE: 0,
    BOOST_DATA_TEST_CASE_F: 1,
};

// The line in a case's doc comment that says why it passes without the change. It starts the
// text of the comment line, after the comment marker and any space.
const STATED = /^Passes before the change:\s*/;
// The line in a case's doc comment that says the code it tests is already in the base, and names
// the commit that brought it there.
const COVERS = /^Covers the base:\s*/;
// A commit named in a Covers the base: line.
const COMMIT = /\b[0-9a-f]{7,40}\b/;
const SOURCE_SUFFIXES = ['.cpp', '.cxx', '.cc', '.c'];

export function isSource(file: string): boolean {
    return SOURCE_SUFFIXES.some((suffix) => file.endsWith(suffix));
}

// The text after the marker in a case's doc comment, or null when no line of it starts with the
// marker. The text may wrap onto the lines that follow, up to the end of the paragraph. The doc
// comment is the comment that ends just before the case's macro: one block comment, or a run
// of // lines.
function markedText(text: string, at: number, marker: RegExp): string | null {
    const before = text.slice(0, at).trimEnd();
    let comment: string;
    if (before.endsWith('*/')) {
        const start = before.lastIndexOf('/*');
        if (start < 0) {
            return null;
        }
        comment = before.slice(start);
    } else {
        const lines = before.split('\n');
        const kept: string[] = [];
        while (lines.length > 0 && lines[lines.length - 1].trim().startsWith('//')) {
            kept.unshift(lines.pop() as string);
        }
        comment = kept.join('\n');
    }
    const lines = comment.split(/\r?\n/).map((line) => line.trim()
        .replace(/^\/\*+|^\/\/+|^\*+(?!\/)/, '').replace(/\*+\/$/, '').trim());
    const first = lines.findIndex((line) => marker.test(line));
    if (first < 0) {
        return null;
    }
    const reason: string[] = [lines[first].replace(marker, '')];
    for (const line of lines.slice(first + 1)) {
        if (line === '') {
            break;
        }
        reason.push(line);
    }
    const joined = reason.join(' ').replace(/\s+/g, ' ').trim();
    return joined === '' ? null : joined;
}

// The commit a Covers the base: line names, or null when it names none.
export function namedCommit(covers: string): string | null {
    return covers.match(COMMIT)?.[0] ?? null;
}

// The --run_test path of every case in a test source, with what its doc comment says about
// passing without the change.
export function casesIn(text: string): Found[] {
    const found: Found[] = [];
    const suites: string[] = [];
    for (const match of blankComments(text).matchAll(TOKEN)) {
        const macro = match[1];
        if (macro === 'BOOST_AUTO_TEST_SUITE_END') {
            suites.pop();
            continue;
        }
        const args = match[2].split(',').map((a) => a.trim());
        const index = NAME_ARGUMENT[macro];
        if (index >= args.length || !/^\w+$/.test(args[index])) {
            continue;
        }
        const name = args[index];
        if (macro.endsWith('_SUITE')) {
            suites.push(name);
        } else {
            found.push({
                path: [...suites, name].join('/'),
                stated: markedText(text, match.index, STATED),
                covers: markedText(text, match.index, COVERS),
            });
        }
    }
    return found;
}

// The commit a Covers the base: line names, in full, when it exists and the base contains it.
// Null otherwise, so that a line naming a commit the change itself made does not excuse a case.
export function inBase(covers: string, base: string): string | null {
    const named = namedCommit(covers);
    if (named === null) {
        return null;
    }
    const found = git(['rev-parse', '--verify', '--quiet', named + '^{commit}'], ROOT, false);
    if (found.code !== 0) {
        return null;
    }
    const commit = found.stdout.trim();
    return git(['merge-base', '--is-ancestor', commit, base], ROOT, false).code === 0 ? commit : null;
}

export function findNewCases(base: string, head: string, changes: Change[]): { cases: Case[]; unregistered: string[] } {
    const headMap = suiteMap(head);
    const baseMap = suiteMap(base);
    const touched = [...new Set(changes.map((c) => c.file).filter((f) => isTestPath(f) && isSource(f)))].sort();

    const collect = (revision: string, mapping: Map<string, SuiteEntry>): Map<string, Map<string, [string, Found]>> => {
        const bySuite = new Map<string, Map<string, [string, Found]>>();
        for (const file of touched) {
            const entry = mapping.get(file);
            if (entry === undefined) {
                continue;
            }
            const text = show(revision, file);
            if (text === null) {
                continue;
            }
            if (!bySuite.has(entry.suite)) {
                bySuite.set(entry.suite, new Map());
            }
            for (const found of casesIn(text)) {
                bySuite.get(entry.suite)!.set(found.path, [file, found]);
            }
        }
        return bySuite;
    };

    const now = collect(head, headMap);
    const before = collect(base, baseMap);
    const cases: Case[] = [];
    for (const suite of [...now.keys()].sort()) {
        for (const [name, [file, found]] of now.get(suite)!) {
            if (!before.get(suite)?.has(name)) {
                const claim = found.covers;
                cases.push({
                    path: name, suite, file, stated: found.stated, claim, covers: claim === null ? null : inBase(claim, base),
                    result: null, note: '',
                });
            }
        }
    }

    const unregistered: string[] = [];
    for (const file of touched) {
        if (!headMap.has(file)) {
            const text = show(head, file);
            if (text !== null && casesIn(text).length > 0) {
                unregistered.push(file);
            }
        }
    }
    return { cases, unregistered };
}
