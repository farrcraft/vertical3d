/**
 * Check the writing rules on the prose a change adds.
 *
 * The rules are the ones in docs/contributing/Conventions.md#writing. The script reads the lines
 * a change adds to Markdown files and to the comments of C++, GLSL, CMake, batch and YAML files.
 * It rebuilds the paragraph around each added line and checks every sentence that overlaps one:
 *
 *   long-sentence    a sentence of more than 35 words. A code span counts as one word.
 *   opener           a sentence that starts with "And" or "So".
 *   inverted         a clause of the form "X, which is what Y" (also how, where, why, when).
 *   personification  "knows", "wants", "owes" or "trusts", or "does not know" and its kin,
 *                    with a subject that is not a person or whoever writes an app.
 *   history          "used to" (not the passive "is used to"), a date, or "phase N". Plans and
 *                    roadmaps are exempt, because they describe change.
 *   long-line        a Markdown prose line longer than 100 columns.
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
 * is a finding. It runs on Node 24 or later, which strips the types, with no dependencies.
 */

import { execFileSync } from 'node:child_process';
import { existsSync, readFileSync, statSync } from 'node:fs';
import { basename } from 'node:path';
import process from 'node:process';

const MAX_WORDS = 35;
const MAX_MD_COLUMNS = 100;
const EXTENDED_MARGIN = 10;
const EXTENDED_BASELINE = 70;

const MARKDOWN_SUFFIXES = ['.md'];
const SLASH_SUFFIXES = ['.h', '.hpp', '.cpp', '.cxx', '.glsl', '.vert', '.frag', '.comp'];
const HASH_SUFFIXES = ['.cmake', '.yml', '.yaml'];
const CMD_SUFFIXES = ['.cmd', '.bat'];

// Historical records, and third-party or generated trees.
const EXCLUDED_PREFIXES = ['vendor/', 'out/', 'docs/plans/completed/', 'docs/roadmap/completed/',
    'docs/audits/'];
// An accepted ADR's body is not edited, so only a new ADR file is checked.
const ADR_BODY = /^docs\/adr\/\d{4}-[^/]*\.md$/;
const HISTORY_EXEMPT_PREFIXES = ['docs/plans/', 'docs/roadmap/'];

const OPENERS = new Set(['And', 'So']);
const INVERTED = /,\s+which\s+is\s+(what|how|where|why|when)\b/i;
const PERSON_VERB =
    /\b([A-Za-z_][\w']*)\s+(?:(?:also|already|only|still|never|always|then|just)\s+)?(knows|wants|owes|trusts)\b/g;
const PERSON_NEGATED =
    /\b([A-Za-z_][\w']*)\s+(?:does\s+not|doesn't|do\s+not|don't|need\s+not|cannot|can't|can)\s+(know|want|owe|trust)\b/g;
const HUMANS = new Set([
    'i', 'you', 'we', 'they', 'he', 'she', 'who', 'one', 'someone', 'anyone', 'everyone', 'nobody',
    'person', 'people', 'reader', 'readers', 'user', 'users', 'developer', 'developers',
    'contributor', 'contributors', 'reviewer', 'reviewers', 'author', 'authors', 'player',
    'players', 'maintainer', 'maintainers', 'artist', 'artists', 'team', 'human', 'humans',
]);
// "An app that wants a camera" names the people who write the app, so these subjects are not
// counted as personified.
const AGENTS = new Set(['app', 'apps', 'consumer', 'consumers', 'caller', 'callers', 'game', 'games',
    'anything', 'something', 'whoever']);
const RELATIVE = /(\w+)\s+(?:else\s+)?(?:that|which)$/;
const USED_TO = /\b([\w']+)\s+used\s+to\b/gi;
const PASSIVE = new Set(['is', 'are', 'was', 'were', 'be', 'been', 'being', 'get', 'gets', 'got',
    'not', 'only', 'also', 'often', 'commonly', 'typically', 'usually', 'and', 'or', 'then',
    "isn't", "aren't"]);
const DATE = new RegExp('\\b\\d{4}-\\d{2}-\\d{2}\\b|\\b(January|February|March|April|May|June|July|'
    + 'August|September|October|November|December)\\s+(\\d{1,2},\\s+)?\\d{4}\\b');
const PHASE = /\bphase\s+\d+\b/i;
const QUOTED = /"[^"]*"|“[^”]*”/g;
const ABBREVIATIONS = new Set(['e.g.', 'i.e.', 'etc.', 'vs.', 'cf.', 'approx.', 'no.', 'fig.', 'eq.',
    'mr.', 'dr.', 'st.', 'ch.', 'sec.', 'al.']);

const LIST_ITEM = /^(\s*)([-*+]|\d+[.)])\s+/;
const METADATA = /^\*\*(Status|Date|Documented in|Amended by|Amends|Superseded by|Supersedes)\*\*:/;
const FENCE = /^\s*(```|~~~)/;
const LINK_ONLY = /^\s*([-*+]\s+)?(\[[^\]]*\]\([^)]*\)|<[^>]+>|https?:\/\/\S+)[.,;:]?\s*$/;
// A comment line that reads as code, such as commented-out code, ends the paragraph around it.
const CODE_LIKE = new RegExp('(;\\s*$|[{}]\\s*$|^\\s*#\\s*(include|define|if|endif|pragma)\\b|'
    + '^\\s*\\w+\\(.*\\)\\s*$|^\\s*(return|if|for|while|auto|const|void|int|float)\\b.*[;({]|->|'
    + '::\\w+\\()');

type Kind = 'md' | 'slash' | 'hash' | 'cmd';

/** One line of a paragraph: its number, the column its prose starts at, and the whole line. */
interface ParagraphLine {
    number: number;
    column: number;
    line: string;
}

interface Finding {
    path: string;
    number: number;
    rule: string;
    detail: string;
}

/** The lines a path adds, or null when the whole file is checked. */
type Added = Set<number> | null;

function git(...args: string[]): string {
    return execFileSync('git', args, { encoding: 'utf8', maxBuffer: 1 << 30, stdio: ['ignore', 'pipe', 'pipe'] });
}

function mergeBase(ref: string | undefined): string {
    const refs = ref === undefined ? ['origin/main', 'main'] : [ref];
    for (const candidate of refs) {
        try {
            return git('merge-base', 'HEAD', candidate).trim();
        } catch {
            continue;
        }
    }
    throw new Error(`no merge base of HEAD with ${refs.join(' or ')}; pass --base`);
}

function columns(text: string): number {
    return Array.from(text).length;
}

function fileKind(path: string): Kind | null {
    const name = basename(path);
    const lower = name.toLowerCase();
    if (MARKDOWN_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'md';
    }
    if (SLASH_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'slash';
    }
    if (name === 'CMakeLists.txt' || HASH_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'hash';
    }
    if (CMD_SUFFIXES.some((suffix) => lower.endsWith(suffix))) {
        return 'cmd';
    }
    return null;
}

function excluded(path: string, newFiles: Set<string>): boolean {
    if (EXCLUDED_PREFIXES.some((prefix) => path.startsWith(prefix)) || path.includes('/vcpkg_installed/')) {
        return true;
    }
    return ADR_BODY.test(path) && !newFiles.has(path);
}

/** Map each changed path to the lines the working tree adds since base, and list the new files. */
function addedLines(base: string): { added: Map<string, Added>; newFiles: Set<string> } {
    const newFiles = new Set<string>();
    for (const line of git('diff', '--name-status', '--no-renames', base).split('\n')) {
        const parts = line.split('\t');
        if (parts[0] === 'A') {
            newFiles.add(parts[parts.length - 1]);
        }
    }
    const added = new Map<string, Added>();
    let current: Set<number> | null = null;
    for (const line of git('diff', '--unified=0', '--no-color', '--no-renames', base).split('\n')) {
        if (line.startsWith('+++ ')) {
            const target = line.slice(4);
            current = null;
            if (target.startsWith('b/')) {
                current = new Set<number>();
                added.set(target.slice(2), current);
            }
        } else if (line.startsWith('@@') && current !== null) {
            const match = /^@@ -\S+ \+(\d+)(?:,(\d+))? @@/.exec(line);
            if (match) {
                const start = Number(match[1]);
                const count = match[2] === undefined ? 1 : Number(match[2]);
                for (let number = start; number < start + count; number++) {
                    current.add(number);
                }
            }
        }
    }
    // A file not yet added to the index is new in its entirety.
    for (const path of git('ls-files', '--others', '--exclude-standard').split('\n')) {
        if (path) {
            newFiles.add(path);
            added.set(path, null);
        }
    }
    return { added, newFiles };
}

function readLines(path: string): string[] {
    return readFileSync(path, 'utf8').split('\n').map((line) => line.replace(/\r$/, ''));
}

// ---------------------------------------------------------------------------------------------
// Paragraphs.

function markdownParagraphs(lines: string[]): ParagraphLine[][] {
    const paragraphs: ParagraphLine[][] = [];
    let current: ParagraphLine[] = [];
    let inFence = false;
    let inComment = false;
    let start = 0;
    if (lines.length > 0 && lines[0].trim() === '---') {
        for (let index = 1; index < lines.length; index++) {
            if (lines[index].trim() === '---') {
                start = index + 1;
                break;
            }
        }
    }
    const flush = (): void => {
        if (current.length > 0) {
            paragraphs.push(current);
            current = [];
        }
    };
    for (let index = start; index < lines.length; index++) {
        const line = lines[index];
        const stripped = line.trim();
        if (FENCE.test(line)) {
            flush();
            inFence = !inFence;
            continue;
        }
        if (inFence) {
            continue;
        }
        if (inComment) {
            inComment = !line.includes('-->');
            continue;
        }
        if (stripped.startsWith('<!--')) {
            flush();
            inComment = !stripped.includes('-->');
            continue;
        }
        if (!stripped || /^[#|<]/.test(stripped) || METADATA.test(stripped)
            || /^(-{3,}|\*{3,}|_{3,})$/.test(stripped)) {
            flush();
            continue;
        }
        let column = 0;
        const quote = /^\s*(>\s?)+/.exec(line);
        if (quote) {
            column = quote[0].length;
        }
        const rest = line.slice(column);
        const item = LIST_ITEM.exec(rest);
        if (item) {
            flush();
            column += item[0].length;
        } else {
            column += rest.length - rest.trimStart().length;
        }
        current.push({ number: index + 1, column, line });
    }
    flush();
    return paragraphs;
}

interface CommentSpan {
    number: number;
    column: number;
    text: string;
    /** False for a comment that follows code on its line. */
    alone: boolean;
}

function commentSpans(lines: string[], kind: Kind): CommentSpan[] {
    const spans: CommentSpan[] = [];
    if (kind === 'slash') {
        let inBlock = false;
        lines.forEach((line, index) => {
            const pieces: Array<[number, string]> = [];
            let position = 0;
            let inString: string | null = null;
            while (position < line.length) {
                if (inBlock) {
                    const end = line.indexOf('*/', position);
                    if (end < 0) {
                        pieces.push([position, line.slice(position)]);
                        position = line.length;
                    } else {
                        pieces.push([position, line.slice(position, end)]);
                        position = end + 2;
                        inBlock = false;
                    }
                    continue;
                }
                const char = line[position];
                if (inString !== null) {
                    if (char === '\\') {
                        position += 2;
                        continue;
                    }
                    if (char === inString) {
                        inString = null;
                    }
                    position += 1;
                    continue;
                }
                if (char === '"' || char === '\'') {
                    inString = char;
                } else if (line.startsWith('//', position)) {
                    pieces.push([position + 2, line.slice(position + 2)]);
                    break;
                } else if (line.startsWith('/*', position)) {
                    inBlock = true;
                    position += 2;
                    continue;
                }
                position += 1;
            }
            for (const [column, text] of pieces) {
                const alone = column === 0 || line.slice(0, column - 2).trim() === '';
                spans.push({ number: index + 1, column, text, alone });
            }
        });
    } else if (kind === 'hash') {
        lines.forEach((line, index) => {
            const match = /^(\s*#|.*\s#)(?!\[)/.exec(line);
            if (match) {
                const column = match[0].length;
                const alone = line.slice(0, column - 1).trim() === '';
                spans.push({ number: index + 1, column, text: line.slice(column), alone });
            }
        });
    } else if (kind === 'cmd') {
        lines.forEach((line, index) => {
            const match = /^\s*(@?rem\b|::)/i.exec(line);
            if (match) {
                const column = match[0].length;
                spans.push({ number: index + 1, column, text: line.slice(column), alone: true });
            }
        });
    }
    return spans;
}

function commentParagraphs(lines: string[], kind: Kind): ParagraphLine[][] {
    const paragraphs: ParagraphLine[][] = [];
    let current: ParagraphLine[] = [];
    let previous: number | null = null;
    let previousAlone = true;
    const flush = (): void => {
        if (current.length > 0) {
            paragraphs.push(current);
            current = [];
        }
    };
    for (const { number, column, text, alone } of commentSpans(lines, kind)) {
        if (previous !== null && (number !== previous + 1 || !alone || !previousAlone)) {
            flush();
        }
        previous = number;
        previousAlone = alone;
        const marker = /^[/*!<\s]*/.exec(text);
        const markerLength = marker ? marker[0].length : 0;
        let body = text.slice(markerLength);
        if (body.trimEnd().endsWith('*')) {
            body = body.replace(/\s*\*+\/?\s*$/, '');
        }
        const stripped = body.trim();
        if (!stripped || /^[-=*/#_~]{3,}$/.test(stripped) || CODE_LIKE.test(stripped)
            || stripped.startsWith('|') || stripped.startsWith('```')) {
            flush();
            continue;
        }
        if (LIST_ITEM.test(body) || stripped.startsWith('@')) {
            flush();
        }
        const start = column + markerLength + (body.length - body.trimStart().length);
        current.push({ number, column: start, line: lines[number - 1] });
    }
    flush();
    return paragraphs;
}

// ---------------------------------------------------------------------------------------------
// Sentences.

/** Return text of the same length with code spans, link targets, URLs and bold markers blanked. */
function mask(text: string): string {
    const units = text.split('');
    const blank = (start: number, end: number, fill: string): void => {
        for (let position = start; position < end; position++) {
            units[position] = fill;
        }
    };
    for (const match of text.matchAll(/(`+)(.+?)\1/g)) {
        blank(match.index, match.index + match[0].length, 'x');
    }
    let masked = units.join('');
    for (const match of masked.matchAll(/!?\[([^\]]*)\]\(([^)]*)\)/g)) {
        const textStart = match.index + match[0].indexOf('[') + 1;
        blank(match.index, textStart, ' ');
        blank(textStart + match[1].length, match.index + match[0].length, ' ');
    }
    masked = units.join('');
    for (const match of masked.matchAll(/<?https?:\/\/[^\s)>]+>?/g)) {
        blank(match.index, match.index + match[0].length, 'x');
    }
    masked = units.join('');
    for (const match of masked.matchAll(/\*\*|__/g)) {
        blank(match.index, match.index + match[0].length, ' ');
    }
    return units.join('');
}

/** Return the [start, end) offsets of each sentence in masked text. */
function splitSentences(masked: string): Array<[number, number]> {
    const sentences: Array<[number, number]> = [];
    let start = 0;
    for (const match of masked.matchAll(/[.!?]["')\]]*(\s+)(?=\S)/g)) {
        const before = masked.slice(0, match.index + 1).split(/\s+/).filter(Boolean);
        const word = before.length > 0 ? before[before.length - 1].toLowerCase() : '';
        // a one-letter word ends a sentence like any other: in this tree it is a variable, such
        // as the x of a vector, far more often than an initial
        if (ABBREVIATIONS.has(word.replace(/^[("']+/, ''))) {
            continue;
        }
        const end = match.index + match[0].length - match[1].length;
        if (masked.slice(start, end).trim()) {
            sentences.push([start, end]);
        }
        start = match.index + match[0].length;
    }
    if (masked.slice(start).trim()) {
        sentences.push([start, masked.length]);
    }
    return sentences;
}

function wordCount(sentence: string): number {
    return sentence.split(/\s+/).filter((token) => /[A-Za-z0-9]/.test(token)).length;
}

/** Return [rule, detail] pairs for one sentence. Matches are reported from the original text. */
function checkSentence(original: string, maskedSentence: string, path: string,
    phaseHeadings: boolean): Array<[string, string]> {
    const findings: Array<[string, string]> = [];
    const words = wordCount(maskedSentence);
    if (words > MAX_WORDS) {
        findings.push(['long-sentence', `${words} words`]);
    }
    const first = /^[\s"'(\[*_>-]*(\w+)/.exec(maskedSentence);
    if (first && OPENERS.has(first[1])) {
        findings.push(['opener', `starts with "${first[1]}"`]);
    }
    // A quoted phrase is cited, not written, so the remaining rules skip it.
    const masked = maskedSentence.replace(QUOTED, (quoted) => 'x'.repeat(quoted.length));
    const at = (index: number, length: number): string => original.slice(index, index + length);
    const inverted = INVERTED.exec(masked);
    if (inverted) {
        findings.push(['inverted', at(inverted.index, inverted[0].length).replace(/^,\s*/, '')]);
    }
    for (const pattern of [PERSON_VERB, PERSON_NEGATED]) {
        for (const match of masked.matchAll(pattern)) {
            let subject = match[1].toLowerCase();
            if (subject === 'that' || subject === 'which') {
                const relative = RELATIVE.exec(masked.slice(0, match.index + match[1].length));
                subject = relative ? relative[1].toLowerCase() : subject;
            }
            if (masked.slice(Math.max(0, match.index - 40), match.index).toLowerCase().includes('whoever')) {
                continue;
            }
            if (!HUMANS.has(subject) && !AGENTS.has(subject)) {
                findings.push(['personification', at(match.index, match[0].length)]);
            }
        }
    }
    if (!HISTORY_EXEMPT_PREFIXES.some((prefix) => path.startsWith(prefix))) {
        for (const match of masked.matchAll(USED_TO)) {
            if (!PASSIVE.has(match[1].toLowerCase())) {
                findings.push(['history', at(match.index, match[0].length)]);
            }
        }
        const date = DATE.exec(masked);
        if (date) {
            findings.push(['history', at(date.index, date[0].length)]);
        }
        const phase = PHASE.exec(masked);
        if (phase && !phaseHeadings) {
            findings.push(['history', at(phase.index, phase[0].length)]);
        }
    }
    return findings;
}

/** Return findings for one paragraph, limited to the sentences that overlap an added line. */
function checkParagraph(paragraph: ParagraphLine[], added: Added, path: string, comment: boolean,
    phaseHeadings: boolean): Finding[] {
    const findings: Finding[] = [];
    const pieces: string[] = [];
    const offsets: Array<[number, number, number]> = [];
    let position = 0;
    for (const { number, column, line } of paragraph) {
        let text = line.slice(column).trimEnd();
        if (comment) {
            text = text.replace(/\s*\*+\/\s*$/, '');
        }
        offsets.push([position, position + text.length, number]);
        pieces.push(text);
        position += text.length + 1;
    }
    const original = pieces.join(' ');
    const masked = mask(original);

    for (const [start, end] of splitSentences(masked)) {
        const numbers = offsets.filter(([low, high]) => low < end && high > start).map(([, , number]) => number);
        if (!numbers.some((number) => added === null || added.has(number))) {
            continue;
        }
        let excerpt = original.slice(start, end).replace(/\s+/g, ' ').trim();
        if (excerpt.length > 90) {
            excerpt = `${excerpt.slice(0, 87)}...`;
        }
        const sentence = original.slice(start, end);
        for (const [rule, detail] of checkSentence(sentence, masked.slice(start, end), path, phaseHeadings)) {
            findings.push({ path, number: numbers[0], rule, detail: `${detail} | ${excerpt}` });
        }
    }

    if (comment && paragraph.length >= 3) {
        for (const { number, line } of paragraph) {
            if (added !== null && !added.has(number)) {
                continue;
            }
            if (/https?:\/\//.test(line)) {
                continue;
            }
            const length = columns(line.trimEnd());
            const others = Math.max(...paragraph.filter((other) => other.number !== number)
                .map((other) => columns(other.line.trimEnd())));
            if (others >= EXTENDED_BASELINE && length > others + EXTENDED_MARGIN) {
                findings.push({ path, number, rule: 'extended-line', detail: `${length} columns against ${others}` });
            }
        }
    }
    return findings;
}

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
    const kind = fileKind(path);
    if (kind === null || excluded(path, newFiles) || !existsSync(path) || !statSync(path).isFile()) {
        return [];
    }
    const lines = readLines(path);
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
                if (length > MAX_MD_COLUMNS && !LINK_ONLY.test(line) && !inLinkTarget(line, MAX_MD_COLUMNS)) {
                    findings.push({ path, number, rule: 'long-line', detail: `${length} columns` });
                }
            }
        }
    } else {
        for (const paragraph of commentParagraphs(lines, kind)) {
            findings.push(...checkParagraph(paragraph, added, path, true, false));
        }
    }
    const unique = new Map<string, Finding>();
    for (const finding of findings) {
        unique.set(`${finding.number}\u0000${finding.rule}\u0000${finding.detail}`, finding);
    }
    return [...unique.values()].sort((a, b) => a.number - b.number || a.rule.localeCompare(b.rule)
        || a.detail.localeCompare(b.detail));
}

function usage(): never {
    process.stderr.write('usage: node scripts/prose.ts [--base REF] | --all FILE...\n');
    process.exit(2);
}

function main(): number {
    const args = process.argv.slice(2);
    let base: string | undefined;
    let all: string[] | null = null;
    for (let index = 0; index < args.length; index++) {
        const arg = args[index];
        if (arg === '--base') {
            base = args[++index];
            if (base === undefined) {
                usage();
            }
        } else if (arg === '--all') {
            all = args.slice(index + 1);
            break;
        } else {
            usage();
        }
    }
    if (all !== null && all.length === 0) {
        usage();
    }

    const findings: Finding[] = [];
    if (all !== null) {
        for (const file of all) {
            const path = file.replace(/\\/g, '/');
            findings.push(...checkFile(path, null, new Set([path])));
        }
    } else {
        process.chdir(git('rev-parse', '--show-toplevel').trim());
        const { added, newFiles } = addedLines(mergeBase(base));
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

process.exitCode = main();
