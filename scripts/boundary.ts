/**
 * Checks the lines a changeset adds for boundary input converted without a check.
 *
 * A value read from a file, the command line or a caller goes through the checked helpers.
 * api/type/Checked.h makes a float into a count or an integer, and api/asset/Json.h reads a
 * member of a JSON object. This script reports two shapes that bypass them, on added lines only:
 *
 *   value-to     any boost::json::value_to<...>. The helpers test a value's type before they
 *                convert it, and value_to throws when the type is wrong.
 *   float-cast   a static_cast to an integer type whose operand contains something that gives a
 *                float: floor, ceil, round, trunc, fmod and their kin, sqrt or pow, .value(),
 *                as_double(), a cast to float or double, or a float literal.
 *
 * Lines in the helper files themselves are not checked, and neither is a test file: a test
 * converts values it chose, not input. Comments and the insides of string and character
 * literals are not read. A line inside a block comment is known from the whole file, not only
 * from the lines the change added.
 *
 * A line the heuristic reports and that is safe for a reason the text does not show carries a
 * trailing comment naming the reason, and is then accepted:
 *
 *   const int cx = static_cast<int>(std::fmod(fx, 256.0f)) & 255;  // checked: fx is finite
 *
 * The reason has to say why the conversion is defined. A clamp alone is not a reason, because
 * std::clamp passes a NaN through.
 *
 * Rule float-cast is a heuristic. It reads the text of the operand, not its type, so it cannot
 * see:
 *   - a float held in a plain variable, such as static_cast<int>(width) where width is a float;
 *   - a C-style cast, a functional cast such as int(x), or an implicit narrowing conversion;
 *   - a call that returns a float without a name on the list above.
 * It also reports an operand that names one of those things without producing a float, such as
 * static_cast<int>(count.value()) where count holds an integer, or a float that is clamped
 * before the cast. A report is a line to read, not a proof of a defect.
 *
 * Usage: node scripts/boundary.ts [--base REF]
 *
 * The changeset is every commit since the merge base with origin/main, or with main when there
 * is no origin/main, plus the working tree and untracked source files. --base names another
 * branch or commit, and the changeset starts at its merge base with HEAD. A --base that names no
 * commit, or that shares no history with HEAD, stops the script with exit status 2.
 *
 * Each report is printed as path:line: rule: excerpt. The exit status is 1 when there is any
 * report, and 0 otherwise.
 *
 * Node runs this file directly: it uses only node: modules, the gates' modules in scripts/lib,
 * and type annotations Node can strip.
 */

import { readFileSync } from 'node:fs';
import process from 'node:process';

import { options, report, runGate } from './lib/cli.ts';
import { changeSince } from './lib/diff.ts';
import { mergeBase } from './lib/git.ts';
import { codeLines } from './lib/lexer.ts';
import { ROOT, isGenerated, isTestPath, isThirdParty } from './lib/tree.ts';

/** An added line: its text, and its code with comments and the insides of literals removed. */
export type Line = { path: string; number: number; text: string; code: string };
export type Report = { path: string; number: number; rule: string; excerpt: string };

const SOURCE = /\.(h|hpp|c|cc|cpp|cxx)$/;

const HELPERS = new Set([
    'api/type/Checked.h',
    'api/type/Checked.cxx',
    'api/asset/Json.h',
    'api/asset/Json.cpp',
]);

/** The comment that accepts a reported line, with the reason the conversion is defined. */
const CHECKED = /\/\/\s*checked:\s*\S/;

const VALUE_TO = /\bvalue_to\s*</;

const INTEGER = '(?:std::)?(?:u?int(?:8|16|32|64)?_t|u?int_(?:fast|least)(?:8|16|32|64)_t|size_t|ptrdiff_t' +
    '|intmax_t|uintmax_t)' +
    '|(?:unsigned|signed)(?:\\s+(?:char|short|int|long(?:\\s+long)?(?:\\s+int)?))?' +
    '|short(?:\\s+int)?|long(?:\\s+long)?(?:\\s+int)?|int';
const CAST = new RegExp('\\bstatic_cast\\s*<\\s*(?:const\\s+)?(?:' + INTEGER + ')\\s*>\\s*\\(', 'g');

const FLOATY = new RegExp([
    '\\b(?:std::)?(?:floor|ceil|round|lround|llround|trunc|fmod|nearbyint|rint|sqrt|pow|exp2?|log2?|log10)f?\\s*\\(',
    '\\.value\\s*\\(\\s*\\)',
    '\\bas_double\\s*\\(',
    '\\bstatic_cast\\s*<\\s*(?:float|double|long\\s+double)\\s*>',
    '(?<![\\w.])(?:\\d+\\.\\d*|\\.\\d+)(?:[eE][+-]?\\d+)?[fFlL]?(?![\\w.])',
    '(?<![\\w.])\\d+[eE][+-]?\\d+[fFlL]?(?![\\w.])',
    '(?<![\\w.])\\d+[fF](?![\\w.])',
].join('|'));

/** Every line a change adds to a C++ source or header since the base, with its code. */
function addedLines(base: string): Line[] {
    const added: Array<{ path: string; number: number; text: string }> = [];
    const contents = new Map<string, string>();
    for (const [path, lines] of changeSince(base).added) {
        if (!SOURCE.test(path)) {
            continue;
        }
        let content: string;
        try {
            content = readFileSync(path, 'utf8');
        } catch {
            // a file the diff names and the working tree lacks has no added lines to read
            continue;
        }
        contents.set(path, content);
        content.split(/\r?\n/).forEach((text, index) => {
            if (lines === null || lines.has(index + 1)) {
                added.push({ path, number: index + 1, text });
            }
        });
    }
    return attachCode(added, (path) => contents.get(path) ?? '');
}

/**
 * Returns the added lines with the code of each, read from the whole file that read returns for
 * its path.
 */
export function attachCode(added: Array<{ path: string; number: number; text: string }>,
    read: (path: string) => string): Line[] {
    // whether a line is code is a property of the whole file: an added line may sit inside a
    // block comment that an unchanged line opened
    const files = new Map<string, string[]>();
    return added.map((line) => {
        if (!files.has(line.path)) {
            files.set(line.path, codeLines(read(line.path)));
        }
        const code = files.get(line.path)?.[line.number - 1] ?? '';
        return { ...line, code };
    });
}

/**
 * Returns the text between the parenthesis at start and its match, and whether the match was
 * found before the end of the text.
 */
function operand(text: string, start: number): { body: string; closed: boolean } {
    let depth = 0;
    for (let index = start; index < text.length; index++) {
        if (text[index] === '(') {
            depth += 1;
        } else if (text[index] === ')') {
            depth -= 1;
            if (depth === 0) {
                return { body: text.slice(start + 1, index), closed: true };
            }
        }
    }
    return { body: text.slice(start + 1), closed: false };
}

/** Returns the reports for a list of added lines, in order. */
export function check(lines: Line[]): Report[] {
    const reports: Report[] = [];
    lines.forEach((line, position) => {
        const { path, number, text, code } = line;
        // a test converts values it chose, not input
        if (HELPERS.has(path) || !SOURCE.test(path) || isTestPath(path) || isThirdParty(path) || isGenerated(path)) {
            return;
        }
        if (CHECKED.test(text)) {
            return;
        }
        if (code.trim() === '') {
            return;
        }
        if (VALUE_TO.test(code)) {
            reports.push({ path, number, rule: 'value-to', excerpt: text.trim() });
        }
        for (const cast of code.matchAll(CAST)) {
            let { body, closed } = operand(code, (cast.index ?? 0) + cast[0].length - 1);
            // an operand that runs onto the next line is read from the added lines that follow
            let following = position + 1;
            while (!closed && following < lines.length && lines[following].path === path &&
                lines[following].number === lines[following - 1].number + 1) {
                ({ body, closed } = operand('(' + body + lines[following].code, 0));
                following += 1;
            }
            if (FLOATY.test(body)) {
                reports.push({ path, number, rule: 'float-cast', excerpt: text.trim() });
                break;
            }
        }
    });
    return reports;
}

function main(): number {
    const { values } = options('usage: node scripts/boundary.ts [--base REF]', { base: { type: 'string' } });
    process.chdir(ROOT);
    const found = check(addedLines(mergeBase(values.base)))
        .map(({ path, number, rule, excerpt }) => ({ path, line: number, rule, message: excerpt }));
    return report(found, 'converts boundary input without a check. Use the helpers in api/type/Checked.h and api/asset/Json.h, or name why the conversion is defined in a // checked: comment.');
}

runGate(import.meta.url, main);
