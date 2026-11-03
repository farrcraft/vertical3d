/**
 * Dividing a paragraph into sentences, with the text a rule must not read blanked.
 */

const ABBREVIATIONS = new Set(['e.g.', 'i.e.', 'etc.', 'vs.', 'cf.', 'approx.', 'no.', 'fig.', 'eq.',
    'mr.', 'dr.', 'st.', 'ch.', 'sec.', 'al.']);

/** Return text of the same length with code spans, link targets, URLs and bold markers blanked. */
export function mask(text: string): string {
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
export function splitSentences(masked: string): Array<[number, number]> {
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

export function wordCount(sentence: string): number {
    return sentence.split(/\s+/).filter((token) => /[A-Za-z0-9]/.test(token)).length;
}
