/**
 * Check the writing rules on the prose a change adds.
 *
 * The script reads the lines a change adds to Markdown files and to the comments of C++, GLSL,
 * TypeScript, JavaScript, CMake, batch and YAML files. It rebuilds the paragraph around each
 * added line and checks every sentence that overlaps one:
 *
 *   long-sentence    a sentence of more than 35 words. A code span counts as one word.
 *   opener           a sentence that starts with "And" or "So".
 *   inverted         a clause of the form "X, which is what Y" (also how, where, why, when), or
 *                    "X is what Y" where a verb follows "what", as in "the submit is what signals
 *                    the fence".
 *   stacked          a sentence with three clause joins, or with two when one is a semicolon or a
 *                    dash, or when both are a relative "which" and one is "and which" or "but
 *                    which". A join is ", which", ", so", "so that", "because", "rather than", a
 *                    semicolon, or a dash between words. A sentence's semicolons count as one
 *                    join, and so do the two dashes around an aside. A semicolon that ends a list
 *                    item is not counted.
 *   personification  "knows", "wants", "owes" or "trusts", or "does not know" and its kin,
 *                    with a subject that is not a person or whoever writes an app.
 *   history          "used to" (not the passive "is used to"), a date, or "phase N". Plans and
 *                    roadmaps are exempt, because they describe change.
 *   long-line        a Markdown prose line, or a comment line with no code before it, longer
 *                    than 100 columns.
 *   extended-line    a comment line more than 10 columns longer than every other line of its
 *                    paragraph, in a paragraph of at least three lines that is wrapped at 70
 *                    columns or more.
 *
 * Code fences, code spans, tables, headings, URLs and front matter are not checked.
 *
 * Usage:
 *   node scripts/prose.ts [--base REF]   check what the working tree adds since the merge base
 *                                        of HEAD with REF (default origin/main, then main)
 *   node scripts/prose.ts --all FILE...  check whole files
 *
 * Each finding prints as "path:line: rule: detail | sentence". The exit status is 1 when there
 * is a finding. It is 2 when the script cannot run: a --base that names no commit or shares no
 * history with HEAD, an empty --base, or a failed git command. It runs on Node 24 or later,
 * which strips the types, and needs only Node's own modules and the gates' modules in scripts/lib.
 */

import { existsSync, statSync } from 'node:fs';
import process from 'node:process';

import { UsageError, options, runGate } from './lib/cli.ts';
import { type Added, changeSince } from './lib/diff.ts';
import { mergeBase } from './lib/git.ts';
import { ROOT } from './lib/tree.ts';
import { columns, excluded, fileKind, readLines } from './prose/files.ts';
import { commentParagraphs, markdownParagraphs } from './prose/paragraphs.ts';
import { type Finding, checkParagraph } from './prose/rules.ts';

export type { Finding } from './prose/rules.ts';

const MAX_COLUMNS = 100;

const LINK_ONLY = /^\s*([-*+]\s+)?(\[[^\]]*\]\([^)]*\)|<[^>]+>|https?:\/\/\S+)[.,;:]?\s*$/;

/** Return whether a line ends with a link whose target crosses the column, and so cannot wrap. */
function inLinkTarget(line: string, column: number): boolean {
    const end = line.trimEnd().length;
    for (const match of line.matchAll(/\]\(([^)\s]*)\)[.,;:]?/g)) {
        if (match.index + 2 <= column && match.index + match[0].length === end) {
            return true;
        }
    }
    return false;
}

function checkFile(path: string, added: Added, newFiles: Set<string>): Finding[] {
    if (fileKind(path) === null || excluded(path, newFiles) || !existsSync(path) || !statSync(path).isFile()) {
        return [];
    }
    return checkLines(path, readLines(path), added);
}

/**
 * Return the findings for the lines of a file, read as the kind its path names. Added holds the
 * numbers of the lines to check, or is null to check them all.
 */
export function checkLines(path: string, lines: string[], added: Added): Finding[] {
    const kind = fileKind(path);
    if (kind === null) {
        return [];
    }
    const findings: Finding[] = [];
    if (kind === 'md') {
        // A document whose own headings name phases, such as a procedure, may refer to them.
        const phaseHeadings = lines.some((line) => /^#+.*\bphase\s+\d/i.test(line));
        for (const paragraph of markdownParagraphs(lines)) {
            findings.push(...checkParagraph(paragraph, added, path, false, phaseHeadings));
            for (const { number, line } of paragraph) {
                if (added !== null && !added.has(number)) {
                    continue;
                }
                const length = columns(line);
                if (length > MAX_COLUMNS && !LINK_ONLY.test(line) && !inLinkTarget(line, MAX_COLUMNS)) {
                    findings.push({ path, number, rule: 'long-line', detail: `${length} columns` });
                }
            }
        }
    } else {
        for (const paragraph of commentParagraphs(lines, kind)) {
            findings.push(...checkParagraph(paragraph, added, path, true, false));
            // A comment after code is held to the code's width, which cpplint checks.
            for (const { number, line, alone } of paragraph) {
                if (!alone || (added !== null && !added.has(number)) || /https?:\/\//.test(line)) {
                    continue;
                }
                const length = columns(line.trimEnd());
                if (length > MAX_COLUMNS) {
                    findings.push({ path, number, rule: 'long-line', detail: `${length} columns` });
                }
            }
        }
    }
    const unique = new Map<string, Finding>();
    for (const finding of findings) {
        unique.set(`${finding.number}\u0000${finding.rule}\u0000${finding.detail}`, finding);
    }
    return [...unique.values()].sort((a, b) => a.number - b.number || a.rule.localeCompare(b.rule)
        || a.detail.localeCompare(b.detail));
}

const USAGE = 'usage: node scripts/prose.ts [--base REF] | --all FILE...';

function main(): number {
    const { values, positionals } = options(USAGE, { base: { type: 'string' }, all: { type: 'boolean' } }, true);
    // --all names the files to check, and nothing else names files
    if (values.all === true ? positionals.length === 0 || values.base !== undefined : positionals.length > 0) {
        throw new UsageError(USAGE);
    }
    const all = values.all === true ? positionals : null;
    const base = values.base;

    const findings: Finding[] = [];
    if (all !== null) {
        for (const file of all) {
            const path = file.replace(/\\/g, '/');
            findings.push(...checkFile(path, null, new Set([path])));
        }
    } else {
        process.chdir(ROOT);
        const { added, newFiles } = changeSince(mergeBase(base));
        for (const path of [...added.keys()].sort()) {
            const lines = added.get(path) ?? null;
            if (lines !== null && lines.size === 0) {
                continue;
            }
            findings.push(...checkFile(path, lines, newFiles));
        }
    }

    for (const { path, number, rule, detail } of findings) {
        process.stdout.write(`${path}:${number}: ${rule}: ${detail}\n`);
    }
    return findings.length > 0 ? 1 : 0;
}

runGate(import.meta.url, main);
