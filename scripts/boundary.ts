// Checks the lines a changeset adds for boundary input converted without a check.
//
// A value read from a file, the command line or a caller goes through the checked helpers.
// api/type/Checked.h makes a float into a count or an integer, and api/asset/Json.h reads a
// member of a JSON object. This script reports two shapes that bypass them, on added lines only:
//
//   value-to     any boost::json::value_to<...>. The helpers test a value's type before they
//                convert it, and value_to throws when the type is wrong.
//   float-cast   a static_cast to an integer type whose operand contains something that gives a
//                float: floor, ceil, round, trunc, fmod and their kin, sqrt or pow, .value(),
//                as_double(), a cast to float or double, or a float literal.
//
// Lines in the helper files themselves are not checked, and neither is a test file: a test
// converts values it chose, not input. Comments and the insides of string and character
// literals are not read. A line inside a block comment is known from the whole file, not only
// from the lines the change added.
//
// A line the heuristic reports and that is safe for a reason the text does not show carries a
// trailing comment naming the reason, and is then accepted:
//
//   const int cx = static_cast<int>(std::fmod(fx, 256.0f)) & 255;  // checked: fx is finite
//
// The reason has to say why the conversion is defined. A clamp alone is not a reason, because
// std::clamp passes a NaN through.
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
// branch or commit, and the changeset starts at its merge base with HEAD. A --base that names no
// commit, or that shares no history with HEAD, stops the script with exit status 2.
//
// Each report is printed as path:line: rule: excerpt. The exit status is 1 when there is any
// report, and 0 otherwise.
//
// Node runs this file directly: it uses only node: modules and type annotations Node can strip.

import { spawnSync } from 'node:child_process';
import { Buffer } from 'node:buffer';
import { readFileSync } from 'node:fs';
import process from 'node:process';

/** An added line: its text, and its code with comments and the insides of literals removed. */
type Line = { path: string; number: number; text: string; code: string };
type Report = { path: string; number: number; rule: string; excerpt: string };

const SOURCE = /\.(h|hpp|c|cc|cpp|cxx)$/;

const HELPERS = new Set([
    'api/type/Checked.h',
    'api/type/Checked.cxx',
    'api/asset/Json.h',
    'api/asset/Json.cpp',
]);

const SKIPPED = ['vendor/', 'out/', 'vcpkg_installed/'];

/** A test file, which converts values it chose rather than input. */
const TEST = /(^|\/)tests\//;

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

/** A raw string literal's opening, with its delimiter. */
const RAW_STRING = /(?<!\w)(?:u8|[uUL])?R"([^()\\\s]{0,16})\(/y;

/**
 * The options that make git print a diff this script can read, whatever the user's
 * configuration. They fix the a/ and b/ prefixes, and turn off colour, external and
 * text-converting drivers, and rename detection.
 */
const DIFF_OPTIONS = ['--src-prefix=a/', '--dst-prefix=b/', '--no-color', '--no-ext-diff', '--no-textconv', '--no-renames'];

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
    if (override !== null) {
        if (git('rev-parse', '--verify', '--quiet', override + '^{commit}') === null) {
            stop(`--base ${override}: not a commit`);
        }
        const base = git('merge-base', 'HEAD', override);
        return base === null ? stop(`--base ${override}: no merge base with HEAD`) : base.trim();
    }
    // the changeset starts where it left the branch it is compared with, so commits that branch
    // has gained since are not counted as the changeset's
    for (const branch of ['origin/main', 'main']) {
        const base = git('merge-base', 'HEAD', branch);
        if (base) {
            return base.trim();
        }
    }
    return stop('no merge base with origin/main or main; pass --base REF');
}

/** The escapes git's C-style quoting of a path uses, other than an octal byte. */
const ESCAPES: Record<string, number> = { a: 7, b: 8, t: 9, n: 10, v: 11, f: 12, r: 13, '"': 34, '\\': 92 };

/**
 * Returns the path a diff header line such as "+++ b/name" names, or null for /dev/null. Git
 * quotes a name that holds a control character, a quote, a backslash or a byte above 127, and
 * ends an unquoted name that holds a space with a tab.
 */
function headerPath(header: string): string | null {
    let name = header.slice(4);
    const quoted = /^"((?:[^"\\]|\\.)*)"/.exec(name);
    if (quoted) {
        const bytes: number[] = [];
        const body = quoted[1];
        for (let index = 0; index < body.length; index++) {
            if (body[index] !== '\\') {
                bytes.push(...Buffer.from(body[index], 'utf8'));
            } else if (/[0-7]{3}/.test(body.slice(index + 1, index + 4))) {
                bytes.push(parseInt(body.slice(index + 1, index + 4), 8));
                index += 3;
            } else {
                index += 1;
                bytes.push(ESCAPES[body[index]] ?? body.charCodeAt(index));
            }
        }
        name = Buffer.from(bytes).toString('utf8');
    } else {
        name = name.replace(/\t$/, '');
    }
    return name.startsWith('b/') ? name.slice(2) : null;
}

/**
 * Returns the code on each line of a C++ file, with comments removed and each string or
 * character literal reduced to its quotes. A block comment and a raw string may span lines, so
 * the file is read whole.
 */
function codeLines(content: string): string[] {
    const out: string[] = [];
    const length = content.length;
    let index = 0;
    while (index < length) {
        const char = content[index];
        RAW_STRING.lastIndex = index;
        const raw = RAW_STRING.exec(content);
        if (content.startsWith('//', index)) {
            const end = content.indexOf('\n', index);
            index = end < 0 ? length : end;
        } else if (content.startsWith('/*', index)) {
            const end = content.indexOf('*/', index + 2);
            const stop = end < 0 ? length : end + 2;
            out.push(' ' + '\n'.repeat(content.slice(index, stop).split('\n').length - 1));
            index = stop;
        } else if (raw) {
            const close = content.indexOf(')' + raw[1] + '"', RAW_STRING.lastIndex);
            const stop = close < 0 ? length : close + raw[1].length + 2;
            out.push('""' + '\n'.repeat(content.slice(index, stop).split('\n').length - 1));
            index = stop;
        } else if (char === '"' || (char === '\'' && !/\b\d[\w']*$/.test(content.slice(Math.max(0, index - 32), index)))) {
            // a quote after the digits of a number is a digit separator, not a character literal
            let end = index + 1;
            while (end < length && content[end] !== char && content[end] !== '\n') {
                end += content[end] === '\\' ? 2 : 1;
            }
            out.push(char + char);
            index = end < length && content[end] === char ? end + 1 : end;
        } else {
            out.push(char);
            index += 1;
        }
    }
    return out.join('').split('\n').map((line) => line.replace(/\r$/, ''));
}

/** Returns every added line in a C++ source or header, with its path and line number. */
function addedLines(base: string): Line[] {
    const diff = git('diff', '--unified=0', ...DIFF_OPTIONS, base);
    if (diff === null) {
        stop('git diff against ' + base + ' failed');
    }
    const added: Array<{ path: string; number: number; text: string }> = [];
    let path: string | null = null;
    let number = 0;
    for (const raw of diff.split(/\r?\n/)) {
        if (raw.startsWith('+++ ')) {
            path = headerPath(raw);
            if (path !== null && !SOURCE.test(path)) {
                path = null;
            }
        } else if (raw.startsWith('@@')) {
            const match = /^@@ -\S+ \+(\d+)/.exec(raw);
            number = match ? Number(match[1]) : 0;
        } else if (raw.startsWith('+') && path !== null) {
            added.push({ path, number, text: raw.slice(1) });
            number += 1;
        }
    }
    const untracked = git('ls-files', '-z', '--others', '--exclude-standard') ?? '';
    for (const name of untracked.split('\0')) {
        if (!SOURCE.test(name)) {
            continue;
        }
        let content: string;
        try {
            content = readFileSync(name, 'utf8');
        } catch {
            continue;
        }
        content.split(/\r?\n/).forEach((text, index) => added.push({ path: name, number: index + 1, text }));
    }
    // whether a line is code is a property of the whole file: an added line may sit inside a
    // block comment that an unchanged line opened
    const files = new Map<string, string[]>();
    return added.map((line) => {
        if (!files.has(line.path)) {
            let content = '';
            try {
                content = readFileSync(line.path, 'utf8');
            } catch {
                // a file the diff names and the working tree lacks has no added lines to read
            }
            files.set(line.path, codeLines(content));
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
function check(lines: Line[]): Report[] {
    const reports: Report[] = [];
    lines.forEach((line, position) => {
        const { path, number, text, code } = line;
        if (HELPERS.has(path) || !SOURCE.test(path) || TEST.test(path) || SKIPPED.some((prefix) => path.startsWith(prefix))) {
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
    const args = process.argv.slice(2);
    const usage = 'usage: node scripts/boundary.ts [--base REF]';
    let base: string | null = null;
    for (let index = 0; index < args.length; index++) {
        if (args[index] === '--base' && index + 1 < args.length) {
            base = args[++index];
        } else if (args[index].startsWith('--base=')) {
            base = args[index].slice('--base='.length);
        } else {
            stop(usage);
        }
        if (base.trim() === '') {
            stop(usage);
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
