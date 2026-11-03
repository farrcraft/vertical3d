/**
 * Split C++ source into code, comments and string or character literals.
 *
 * The review gates read C++ and GLSL through this module, so a // or /* inside a literal starts
 * no comment, and a quote inside a comment opens no literal, in every gate alike.
 *
 * The rules it applies:
 *
 *   line comment     // to the end of its line. The line break is not part of it.
 *   block comment    from its opening marker to the first closing marker after it, across
 *                    lines, or to the end of the text when it is not closed.
 *   raw string       an optional u8, u, U or L prefix, then R", a delimiter of up to 16
 *                    characters and an opening parenthesis. It ends at the first closing
 *                    parenthesis followed by the same delimiter and a quote, across lines, or at
 *                    the end of the text. The prefix must not follow a letter, digit or
 *                    underscore, so fooR"( is an identifier and an ordinary string.
 *   literal          a string or character literal from its opening quote to the matching
 *                    quote. A backslash escapes the next character. A backslash before a line
 *                    break, LF or CRLF, continues the literal onto the next line. A literal
 *                    that reaches a line break without a backslash ends there. An encoding
 *                    prefix such as u8 or L before an ordinary literal is code.
 *   digit separator  a quote between two digits of a number literal, as in 1'000 or 0x1'F. It
 *                    is code, not the start of a character literal.
 *
 * Preprocessor directives get no special treatment, and neither does a backslash at the end of a
 * line comment.
 *
 * Line numbers are exact. Lines are the text split at LF, so line N of every result is line N of
 * the file, including inside a continued literal, a raw string or a block comment. A carriage
 * return before a line break belongs to no segment. Columns are offsets in UTF-16 code units
 * from the start of the line, as JavaScript indexes a string.
 *
 * Node runs this file directly. It imports nothing, and Node strips its type annotations.
 */

export type Kind = 'code' | 'comment' | 'literal';

/** A run of one kind in the whole text, as offsets [start, end). */
export interface Token {
    kind: Kind;
    start: number;
    end: number;
}

/** The part of a token on one line. */
export interface Segment {
    kind: Kind;
    /** The offset of the segment in its line. */
    column: number;
    text: string;
    /** True when the segment holds the token's first character. */
    opens: boolean;
    /** True when the token ends on this line. */
    closes: boolean;
}

/** The body of a comment on one line, without its // or its block markers. */
export interface CommentText {
    column: number;
    text: string;
}

/** A raw string literal's opening, with its delimiter. */
export const RAW_STRING = /(?<!\w)(?:u8|[uUL])?R"([^()\\\s]{0,16})\(/y;

/**
 * Returns whether the quote at index is a digit separator. It is one only between two digits of
 * a number literal: the token before it starts with a digit, and the characters on both sides
 * are digits of that number's base. Anything else, such as the quote after u8, opens a character
 * literal.
 */
export function isDigitSeparator(text: string, index: number): boolean {
    let start = index;
    while (start > 0 && /[\w.']/.test(text[start - 1])) {
        start -= 1;
    }
    const token = text.slice(start, index);
    if (!/^\.?\d/.test(token)) {
        return false;
    }
    const digit = /^0[xX]/.test(token) ? /[0-9A-Fa-f]/ : /[0-9]/;
    return digit.test(text[index - 1] ?? '') && digit.test(text[index + 1] ?? '');
}

/** Returns the tokens of the text, in order. Together they cover every character once. */
export function tokens(text: string): Token[] {
    const out: Token[] = [];
    const length = text.length;
    let code = 0;
    const push = (kind: Kind, start: number, end: number): void => {
        if (code < start) {
            out.push({ kind: 'code', start: code, end: start });
        }
        out.push({ kind, start, end });
        code = end;
    };
    let index = 0;
    while (index < length) {
        const char = text[index];
        RAW_STRING.lastIndex = index;
        const raw = 'uULR'.includes(char) ? RAW_STRING.exec(text) : null;
        if (text.startsWith('//', index)) {
            const end = text.indexOf('\n', index);
            const stop = end < 0 ? length : end;
            push('comment', index, stop);
            index = stop;
        } else if (text.startsWith('/*', index)) {
            const end = text.indexOf('*/', index + 2);
            const stop = end < 0 ? length : end + 2;
            push('comment', index, stop);
            index = stop;
        } else if (raw) {
            const close = text.indexOf(')' + raw[1] + '"', RAW_STRING.lastIndex);
            const stop = close < 0 ? length : close + raw[1].length + 2;
            push('literal', index, stop);
            index = stop;
        } else if (char === '"' || (char === '\'' && !isDigitSeparator(text, index))) {
            let end = index + 1;
            while (end < length && text[end] !== char && text[end] !== '\n') {
                if (text[end] === '\\') {
                    end += text.startsWith('\r\n', end + 1) ? 3 : 2;
                } else {
                    end += 1;
                }
            }
            const stop = end < length && text[end] === char ? end + 1 : Math.min(end, length);
            push('literal', index, stop);
            index = stop;
        } else {
            index += 1;
        }
    }
    if (code < length) {
        out.push({ kind: 'code', start: code, end: length });
    }
    return out;
}

/**
 * Returns the segments on each line of the text, one array per line. A comment or a literal
 * that covers a line has a segment there even when it is empty on that line. An empty run of
 * code has none.
 */
export function lex(text: string): Segment[][] {
    const lines: Segment[][] = text.split('\n').map(() => []);
    let line = 0;
    let lineStart = 0;
    for (const { kind, start, end } of tokens(text)) {
        const pieces = text.slice(start, end).split('\n');
        let offset = start;
        pieces.forEach((piece, index) => {
            const last = index === pieces.length - 1;
            let body = piece;
            if ((!last || text[end] === '\n') && body.endsWith('\r')) {
                body = body.slice(0, -1);
            }
            if (body !== '' || kind !== 'code') {
                const column = index === 0 ? start - lineStart : 0;
                lines[line].push({ kind, column, text: body, opens: index === 0, closes: last });
            }
            offset += piece.length + 1;
            if (!last) {
                line += 1;
                lineStart = offset;
            }
        });
    }
    return lines;
}

/**
 * Returns the comment bodies on each line. A line comment's body follows its //. A block
 * comment's body on its first line follows its opening marker, and on its last line ends before
 * its closing marker. Each line inside a block comment has a body, which may be empty.
 */
export function commentLines(text: string): CommentText[][] {
    return lex(text).map((segments) => segments.filter((s) => s.kind === 'comment').map((s) => {
        let column = s.column;
        let body = s.text;
        const block = !s.opens || body.startsWith('/*');
        if (s.opens) {
            column += 2;
            body = body.slice(2);
        }
        if (block && s.closes && body.endsWith('*/')) {
            body = body.slice(0, -2);
        }
        return { column, text: body };
    }));
}

/**
 * Returns the code on each line, with comments removed and each literal reduced to its quotes.
 * A block comment becomes one space on the line it opens on. A literal becomes "" or '' on the
 * line it opens on; a raw string becomes "" and loses its prefix. A line inside a block comment
 * or a literal keeps only the code outside it.
 */
export function codeLines(text: string): string[] {
    return lex(text).map((segments) => segments.map((s) => {
        if (s.kind === 'code') {
            return s.text;
        }
        if (!s.opens) {
            return '';
        }
        if (s.kind === 'comment') {
            return s.text.startsWith('/*') ? ' ' : '';
        }
        const quote = s.text[0] === '\'' ? '\'' : '"';
        return quote + quote;
    }).join(''));
}

/**
 * Returns the text with every character of every comment replaced by a space, except line
 * breaks. An offset in the result is the same offset in the text, and literals are unchanged.
 */
export function blankComments(text: string): string {
    const out = text.split('');
    for (const { kind, start, end } of tokens(text)) {
        if (kind !== 'comment') {
            continue;
        }
        for (let index = start; index < end; index++) {
            if (out[index] !== '\n') {
                out[index] = ' ';
            }
        }
    }
    return out.join('');
}
