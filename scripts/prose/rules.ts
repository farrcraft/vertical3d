/**
 * The writing rules the prose gate checks, on each sentence and line of a paragraph.
 */

import { columns } from './files.ts';
import type { Added } from '../lib/diff.ts';
import type { ParagraphLine } from './paragraphs.ts';
import { mask, splitSentences, wordCount } from './sentences.ts';

const MAX_WORDS = 35;
const EXTENDED_MARGIN = 10;
const EXTENDED_BASELINE = 70;

const HISTORY_EXEMPT_PREFIXES = ['docs/plans/', 'docs/roadmap/', 'CHANGELOG.md'];

const OPENERS = new Set(['And', 'So']);
const INVERTED = /,\s+which\s+is\s+(what|how|where|why|when)\b/i;
// "X is what signals Y": "what" followed by a verb, not by the subject of a clause. A word that
// ends in s and is followed by "(" or "of" is taken as a name or a noun.
const CLEFT = /\b(?:is|are|was|were)\s+what\s+(?:also\s+|only\s+|then\s+)?([a-z]+s)\b(?![(\w]|\s+of\b)/gi;
const NOT_VERBS = new Set(['this', 'its', 'his', 'is', 'was', 'has', 'as', 'us', 'thus', 'those', 'these', 'ones']);
// A clause join: a relative "which", ", so" or "so that", "because", "rather than", a semicolon,
// or a dash between two words. A dash between operands, as in "n - 1", is not one.
const JOIN = new RegExp([
    /,\s+which\b/, /\b(?:but|and)\s+which\b/, /,\s+so\b(?!\s+(?:does|do|is|are|can|did|will)\b)/,
    /\bso\s+that\b/, /\bbecause\b/, /\brather\s+than\b/, /;/,
    /(?<=(?:^|\s)[A-Za-z][a-z']+,?)\s(?:-{1,2}|–|—)\s(?=[A-Za-z][a-z']*(?:[\s,.;:]|$))/, /(?<=[a-z])—(?=[a-z])/,
].map((part) => part.source).join('|'), 'gi');
const DASH = /^[-–—]+$/;
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

export interface Finding {
    path: string;
    number: number;
    rule: string;
    detail: string;
}

/**
 * Return the clause joins of a sentence, each named as the stacked rule reports it. A semicolon
 * that ends the sentence closes a list item and is not counted. The semicolons of one sentence
 * count as one join, and so do the two dashes around an aside.
 */
function clauseJoins(masked: string): string[] {
    const joins: string[] = [];
    let semicolons = 0;
    let dashes = 0;
    for (const match of masked.replace(/;\s*$/, '').matchAll(JOIN)) {
        const join = match[0].replace(/^,/, '').trim().replace(/\s+/g, ' ').toLowerCase();
        if (join === ';') {
            semicolons += 1;
            if (semicolons === 1) {
                joins.push('a semicolon');
            }
        } else if (DASH.test(join)) {
            dashes += 1;
            if (dashes % 2 === 1) {
                joins.push('a dash');
            }
        } else {
            joins.push(`"${join}"`);
        }
    }
    return joins;
}

/** Return [rule, detail] pairs for one sentence. Matches are reported from the original text. */
function checkSentence(original: string, maskedSentence: string, path: string,
    phaseHeadings: boolean): Array<[string, string]> {
    const findings: Array<[string, string]> = [];
    const words = wordCount(maskedSentence);
    if (words > MAX_WORDS) {
        findings.push(['long-sentence', `${words} words`]);
    }
    const first = /^[\s"'(\[*_>-]*(\w+)/.exec(maskedSentence)?.[1];
    if (first !== undefined && OPENERS.has(first)) {
        findings.push(['opener', `starts with "${first}"`]);
    }
    // A quoted phrase is cited, not written, so the remaining rules skip it.
    const masked = maskedSentence.replace(QUOTED, (quoted) => 'x'.repeat(quoted.length));
    const at = (index: number, length: number): string => original.slice(index, index + length);
    const joins = clauseJoins(masked);
    const strong = joins.some((join) => join === 'a semicolon' || join === 'a dash');
    const relatives = joins.filter((join) => join.endsWith('which"')).length;
    const stackedWhich = relatives >= 2 && joins.some((join) => /^"(and|but) which"$/.test(join));
    if (joins.length >= 3 || (joins.length === 2 && (strong || stackedWhich))) {
        findings.push(['stacked', `${joins.length} joins: ${joins.join(', ')}`]);
    }
    const inverted = INVERTED.exec(masked);
    if (inverted) {
        findings.push(['inverted', at(inverted.index, inverted[0].length).replace(/^,\s*/, '')]);
    }
    for (const cleft of masked.matchAll(CLEFT)) {
        if (!NOT_VERBS.has((cleft[1] ?? '').toLowerCase())) {
            findings.push(['inverted', at(cleft.index, cleft[0].length)]);
        }
    }
    for (const pattern of [PERSON_VERB, PERSON_NEGATED]) {
        for (const match of masked.matchAll(pattern)) {
            const word = match[1] ?? '';
            let subject = word.toLowerCase();
            if (subject === 'that' || subject === 'which') {
                subject = RELATIVE.exec(masked.slice(0, match.index + word.length))?.[1]?.toLowerCase() ?? subject;
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
            if (!PASSIVE.has((match[1] ?? '').toLowerCase())) {
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
export function checkParagraph(paragraph: ParagraphLine[], added: Added, path: string, comment: boolean,
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
            findings.push({ path, number: numbers[0] ?? 0, rule, detail: `${detail} | ${excerpt}` });
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
