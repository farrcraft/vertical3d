// Checks the lines a changeset adds for boundary input converted without a check.
//
// A value read from a file, the command line or a caller goes through the checked helpers:
// api/type/Checked.h for a float made into a count or an integer, and api/asset/Json.h for a
// member of a JSON object. This script reports two shapes that bypass them, on added lines only:
//
//   value-to     any boost::json::value_to<...>. The helpers test a value's type before they
//                convert it, and value_to throws when the type is wrong.
//   float-cast   a static_cast to an integer type whose operand contains something that gives a
//                float: floor, ceil, round, trunc, fmod and their kin, sqrt or pow, .value(),
//                as_double(), a cast to float or double, or a float literal.
//
// Lines in the helper files themselves are not checked, and neither is a comment.
//
// Rule float-cast is a heuristic. It reads the text of the operand, not its type, so it cannot
// see:
//   - a float held in a plain variable, such as static_cast<int>(width) where width is a float;
//   - a C-style cast, a functional cast such as int(x), or an implicit narrowing conversion;
//   - a call that returns a float without a name on the list above.
// It also reports an operand that names one of those things without producing a float, such as
// static_cast<int>(count.value()) where count holds an integer, or a float that is clamped
// before the cast. A report is a line to read, not a proof of a defect.
//
// Usage: node scripts/boundary.ts [--base REF]
//
// The changeset is every commit since the merge base with origin/main, or with main when there
// is no origin/main, plus the working tree and untracked source files. --base names another
// commit to compare against. Each report is printed as path:line: rule: excerpt. The exit status
// is 1 when there is any report, and 0 otherwise.
//
// Node runs this file directly: it uses only node: modules and type annotations Node can strip.

import { spawnSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import process from 'node:process';

type Line = { path: string; number: number; text: string };
type Report = { path: string; number: number; rule: string; excerpt: string };

const SOURCE = /\.(h|hpp|c|cc|cpp|cxx)$/;

const HELPERS = new Set([
    'api/type/Checked.h',
    'api/type/Checked.cxx',
    'api/asset/Json.h',
    'api/asset/Json.cpp',
]);

const SKIPPED = ['vendor/', 'out/', 'vcpkg_installed/'];

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

/** Runs git and returns its standard output, or null when it fails. */
function git(...args: string[]): string | null {
    const result = spawnSync('git', args, { encoding: 'utf8', maxBuffer: 1 << 30 });
    if (result.status !== 0 || result.error) {
        return null;
    }
    return result.stdout;
}

function stop(message: string): never {
    process.stderr.write('boundary.ts: ' + message + '\n');
    process.exit(2);
}

/** Returns the commit the changeset is measured from. */
function mergeBase(override: string | null): string {
    if (override) {
        return override;
    }
    for (const branch of ['origin/main', 'main']) {
        const base = git('merge-base', 'HEAD', branch);
        if (base) {
            return base.trim();
        }
    }
    return stop('no merge base with origin/main or main; pass --base REF');
}

/** Returns every added line in a C++ source or header, with its path and line number. */
function addedLines(base: string): Line[] {
    const diff = git('diff', '--unified=0', '--no-color', '--no-ext-diff', '--no-renames', base);
    if (diff === null) {
        stop('git diff against ' + base + ' failed');
    }
    const lines: Line[] = [];
    let path: string | null = null;
    let number = 0;
    for (const raw of diff.split(/\r?\n/)) {
        if (raw.startsWith('+++ ')) {
            const name = raw.slice(4);
            path = name.startsWith('b/') ? name.slice(2) : null;
        } else if (raw.startsWith('@@')) {
            const match = /^@@ -\S+ \+(\d+)/.exec(raw);
            number = match ? Number(match[1]) : 0;
        } else if (raw.startsWith('+') && path !== null) {
            lines.push({ path, number, text: raw.slice(1) });
            number += 1;
        }
    }
    const untracked = git('ls-files', '--others', '--exclude-standard') ?? '';
    for (const name of untracked.split(/\r?\n/)) {
        if (!SOURCE.test(name)) {
            continue;
        }
        let content: string;
        try {
            content = readFileSync(name, 'utf8');
        } catch {
            continue;
        }
        content.split(/\r?\n/).forEach((text, index) => lines.push({ path: name, number: index + 1, text }));
    }
    return lines;
}

/** Returns the code on a line, without a // comment, or nothing for a line of a block comment. */
function stripComment(text: string): string {
    const stripped = text.trimStart();
    if (stripped.startsWith('*') || stripped.startsWith('/*') || stripped.startsWith('//')) {
        return '';
    }
    const cut = text.indexOf('//');
    return cut < 0 ? text : text.slice(0, cut);
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
function check(lines: Line[]): Report[] {
    const reports: Report[] = [];
    lines.forEach((line, position) => {
        const { path, number, text } = line;
        if (HELPERS.has(path) || !SOURCE.test(path) || SKIPPED.some((prefix) => path.startsWith(prefix))) {
            return;
        }
        const code = stripComment(text);
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
                ({ body, closed } = operand('(' + body + stripComment(lines[following].text), 0));
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
    const args = process.argv.slice(2);
    let base: string | null = null;
    for (let index = 0; index < args.length; index++) {
        if (args[index] === '--base' && index + 1 < args.length) {
            base = args[++index];
        } else if (args[index].startsWith('--base=')) {
            base = args[index].slice('--base='.length);
        } else {
            stop('usage: node scripts/boundary.ts [--base REF]');
        }
    }
    const root = git('rev-parse', '--show-toplevel');
    if (root === null) {
        stop('not inside a git repository');
    }
    process.chdir(root.trim());
    const reports = check(addedLines(mergeBase(base)));
    for (const report of reports) {
        process.stdout.write(`${report.path}:${report.number}: ${report.rule}: ${report.excerpt}\n`);
    }
    return reports.length > 0 ? 1 : 0;
}

process.exitCode = main();
