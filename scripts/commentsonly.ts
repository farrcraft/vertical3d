/**
 * Check that a change moved nothing but comments and whitespace.
 *
 * The script compares each changed C++, GLSL and CMake file in the working tree with the same file
 * at a base revision. It removes the comments from both, collapses each run of whitespace outside
 * a literal to one space, and drops blank lines. A file whose remaining lines differ has changed
 * in code. Line breaks are kept, because a preprocessor directive ends at one.
 *
 * It reports:
 *
 *   code-changed     the first code line that differs, with its text at the base and now.
 *   stray-close      a block comment's closing marker in code that the base does not have.
 *                    It is usually the end of a comment that closed early.
 *   continued        a line comment that ends with a backslash, which the base does not
 *                    have. The compiler joins the next line to the comment, and the lexer
 *                    the check uses does not, so the code on that line would be hidden.
 *   added, deleted   a checked file that the change adds or removes. Untracked files count.
 *
 * Other files are not read, so a change to a document or a script passes.
 *
 * Usage:
 *   node scripts/commentsonly.ts [--base REF]   compare the working tree with REF (default HEAD)
 *
 * Each finding prints as "path:line: rule: detail". The line is in the working tree's file, or 0
 * when the file is gone. The exit status is 1 when there is a finding, and 2 when the script
 * cannot run.
 */

import { execFileSync } from 'node:child_process';
import { readFileSync } from 'node:fs';
import process from 'node:process';

import { shouldRun } from './entry.ts';
import { lex } from './lexer.ts';

const SLASH_SUFFIXES = ['.h', '.hpp', '.cpp', '.cxx', '.glsl', '.vert', '.frag', '.comp'];
const HASH_SUFFIXES = ['.cmake'];

// These options keep the output the same under any user's git configuration.
const DIFF_OPTIONS = ['--no-color', '--no-ext-diff', '--no-textconv', '--no-renames'];

/** One line of code that is left once the comments are removed, and its line in the file. */
export interface CodeLine {
    number: number;
    text: string;
}

export interface Finding {
    path: string;
    number: number;
    rule: string;
    detail: string;
}

/** A reason the script cannot run, printed as one line with exit status 2. */
class Stop extends Error {}

function git(...args: string[]): string {
    try {
        return execFileSync('git', args, { encoding: 'utf8', maxBuffer: 1 << 30, stdio: ['ignore', 'pipe', 'pipe'] });
    } catch (error) {
        const stderr = String((error as { stderr?: unknown }).stderr ?? '').trim().split('\n')[0];
        throw new Stop(`git ${args.join(' ')} failed${stderr ? ': ' + stderr : ''}`);
    }
}

/** Whether the script reads a path, and which comment syntax it uses. */
export function syntax(path: string): 'slash' | 'hash' | null {
    const lower = path.toLowerCase();
    if (SLASH_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'slash';
    }
    if (lower === 'cmakelists.txt' || lower.endsWith('/cmakelists.txt')
        || HASH_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'hash';
    }
    return null;
}

/** Collapse whitespace and drop the lines left empty. */
function keep(lines: string[]): CodeLine[] {
    const kept: CodeLine[] = [];
    lines.forEach((line, index) => {
        const text = line.trim();
        if (text !== '') {
            kept.push({ number: index + 1, text });
        }
    });
    return kept;
}

/**
 * The code lines of C++ or GLSL source. A block comment becomes one space, so that removing one
 * cannot join two tokens. A literal is kept exactly, whitespace included.
 */
export function slashCode(text: string): CodeLine[] {
    return keep(lex(text.replace(/\r\n/g, '\n')).map((segments) => {
        let line = '';
        let pending = '';
        for (const s of segments) {
            if (s.kind === 'literal') {
                line += pending.replace(/\s+/g, ' ') + s.text;
                pending = '';
            } else if (s.kind === 'code') {
                pending += s.text;
            } else if (s.opens && s.text.startsWith('/*')) {
                pending += ' ';
            }
        }
        return line + pending.replace(/\s+/g, ' ');
    }));
}

/**
 * The code lines of a CMake file. A # outside a quoted argument starts a comment to the end of
 * its line, and #[[ ]] or #[=[ ]=] is a bracket comment, which may span lines.
 */
export function hashCode(text: string): CodeLine[] {
    // a bracket comment is replaced by its line breaks, so the lines after it keep their numbers
    const unbracketed = text.replace(/\r\n/g, '\n')
        .replace(/#\[(=*)\[[\s\S]*?\]\1\]/g, (comment) => ' ' + comment.replace(/[^\n]/g, ''));
    return keep(unbracketed.split('\n').map((line) => {
        let quoted = false;
        for (let i = 0; i < line.length; i++) {
            const c = line[i];
            if (c === '\\') {
                i++;
            } else if (c === '"') {
                quoted = !quoted;
            } else if (c === '#' && !quoted) {
                return line.slice(0, i).replace(/\s+/g, ' ');
            }
        }
        return line.replace(/\s+/g, ' ');
    }));
}

/** The line comments that end with a backslash, and the lines they are on. */
function continuedLines(text: string): CodeLine[] {
    const found: CodeLine[] = [];
    lex(text).forEach((segments, index) => {
        const last = segments.at(-1);
        if (last !== undefined && last.kind === 'comment' && last.opens && last.text.startsWith('//')
            && /\\\s*$/.test(last.text)) {
            found.push({ number: index + 1, text: last.text });
        }
    });
    return found;
}

/** The findings for one file that is changed, from its text at the base and now. */
export function compare(path: string, was: string, now: string): Finding[] {
    const code = syntax(path) === 'hash' ? hashCode : slashCode;
    const before = code(was);
    const after = code(now);
    const findings: Finding[] = [];
    const length = Math.min(before.length, after.length);
    let index = 0;
    while (index < length && before[index].text === after[index].text) {
        index++;
    }
    if (index < before.length || index < after.length) {
        const old = index < before.length ? before[index].text : '(end of file)';
        const fresh = index < after.length ? after[index].text : '(end of file)';
        const number = index < after.length ? after[index].number : (after.at(-1)?.number ?? 0);
        findings.push({ path, number, rule: 'code-changed', detail: `base: ${old} | now: ${fresh}` });
    }
    // a line the base has with the same text is not reported, so a sweep is not blamed for it
    const added = (rule: string, base: CodeLine[], head: CodeLine[]) => {
        const known = new Set(base.map((line) => line.text));
        for (const line of head.filter((line) => !known.has(line.text))) {
            findings.push({ path, number: line.number, rule, detail: line.text });
        }
    };
    if (code === slashCode) {
        added('continued', continuedLines(was), continuedLines(now));
        const closes = (lines: CodeLine[]) => lines.filter((line) => line.text.includes('*/'));
        added('stray-close', closes(before), closes(after));
    }
    return findings;
}

/** The checked files that differ from the base, with the status git gives each. */
function changed(base: string): Array<{ status: string; path: string }> {
    const files: Array<{ status: string; path: string }> = [];
    const fields = git('diff', '--name-status', '-z', ...DIFF_OPTIONS, base).split('\0');
    for (let index = 0; index + 1 < fields.length; index += 2) {
        files.push({ status: fields[index], path: fields[index + 1] });
    }
    for (const path of git('ls-files', '-z', '--others', '--exclude-standard').split('\0')) {
        if (path) {
            files.push({ status: 'A', path });
        }
    }
    return files.filter((file) => syntax(file.path) !== null).sort((a, b) => a.path.localeCompare(b.path));
}

function usage(): never {
    process.stderr.write('usage: node scripts/commentsonly.ts [--base REF]\n');
    process.exit(2);
}

function main(): number {
    const args = process.argv.slice(2);
    let base = 'HEAD';
    if (args.length === 2 && args[0] === '--base' && args[1].trim() !== '') {
        base = args[1];
    } else if (args.length === 1 && args[0].startsWith('--base=') && args[0].length > '--base='.length) {
        base = args[0].slice('--base='.length);
    } else if (args.length !== 0) {
        usage();
    }

    process.chdir(git('rev-parse', '--show-toplevel').trim());
    try {
        git('rev-parse', '--verify', '--quiet', `${base}^{commit}`);
    } catch {
        throw new Stop(`--base ${base}: not a commit`);
    }

    const findings: Finding[] = [];
    for (const { status, path } of changed(base)) {
        if (status === 'A') {
            findings.push({ path, number: 1, rule: 'added', detail: 'a new file is a code change' });
        } else if (status === 'D') {
            findings.push({ path, number: 0, rule: 'deleted', detail: 'a removed file is a code change' });
        } else {
            findings.push(...compare(path, git('show', `${base}:${path}`), readFileSync(path, 'utf8')));
        }
    }
    for (const { path, number, rule, detail } of findings) {
        process.stdout.write(`${path}:${number}: ${rule}: ${detail}\n`);
    }
    return findings.length > 0 ? 1 : 0;
}

if (shouldRun(import.meta.url)) {
    try {
        process.exitCode = main();
    } catch (error) {
        if (!(error instanceof Stop)) {
            throw error;
        }
        process.stderr.write(`commentsonly.ts: ${error.message}\n`);
        process.exitCode = 2;
    }
}
