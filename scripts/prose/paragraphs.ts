/**
 * Rebuilding the paragraphs of a file: the prose of a Markdown file, and the comments of a
 * source file.
 */

import { commentLines } from '../lib/lexer.ts';
import { regions } from '../lib/script.ts';
import type { Kind } from './files.ts';

export const LIST_ITEM = /^(\s*)([-*+]|\d+[.)])\s+/;

const METADATA = /^\*\*(Status|Date|Documented in|Amended by|Amends|Superseded by|Supersedes)\*\*:/;
const FENCE = /^\s*(```|~~~)/;

// A comment line that reads as code, such as commented-out code, ends the paragraph around it.
const CODE_LIKE = new RegExp('(;\\s*$|[{}]\\s*$|^\\s*#\\s*(include|define|if|endif|pragma)\\b|'
    + '^\\s*\\w+\\(.*\\)\\s*$|^\\s*(return|if|for|while|auto|const|void|int|float)\\b.*[;({]|->|'
    + '::\\w+\\()');

/**
 * One line of a paragraph: its number, the column its prose starts at, and the whole line. Alone
 * is true for a comment line with no code before the comment.
 */
export interface ParagraphLine {
    number: number;
    column: number;
    line: string;
    alone?: boolean;
}

export function markdownParagraphs(lines: string[]): ParagraphLine[][] {
    const paragraphs: ParagraphLine[][] = [];
    let current: ParagraphLine[] = [];
    let inFence = false;
    let inComment = false;
    let start = 0;
    if (lines[0]?.trim() === '---') {
        const close = lines.findIndex((line, index) => index > 0 && line.trim() === '---');
        if (close > 0) {
            start = close + 1;
        }
    }
    const flush = (): void => {
        if (current.length > 0) {
            paragraphs.push(current);
            current = [];
        }
    };
    for (const [index, line] of lines.entries()) {
        if (index < start) {
            continue;
        }
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

/**
 * Return the comments of a TypeScript or JavaScript file, without their markers. Strings,
 * template literals and regular expression literals are skipped, so a // or /* inside one starts
 * no comment.
 */
export function scriptSpans(lines: string[]): CommentSpan[] {
    const text = lines.join('\n');
    const ranges = regions(text).filter((region) => region.kind === 'comment').map(({ start, end }): [number, number] => {
        // a closed block comment ends with its marker, which is not part of the text
        const closed = text.startsWith('/*', start) && end - start >= 4 && text.startsWith('*/', end - 2);
        return [start + 2, closed ? end - 2 : end];
    });
    // each line with the offset it starts at in the text
    const rows: Array<{ line: string; start: number }> = [];
    let offset = 0;
    for (const line of lines) {
        rows.push({ line, start: offset });
        offset += line.length + 1;
    }
    const spans: CommentSpan[] = [];
    let row = 0;
    for (const [from, to] of ranges) {
        while ((rows[row + 1]?.start ?? Infinity) <= from) {
            row += 1;
        }
        for (let at = row; at < rows.length; at++) {
            const { line, start } = rows[at] ?? { line: '', start: Infinity };
            if (start > to) {
                break;
            }
            const column = Math.max(from, start) - start;
            const end = Math.min(to, start + line.length) - start;
            const alone = column === 0 || line.slice(0, column - 2).trim() === '';
            spans.push({ number: at + 1, column, text: line.slice(column, end), alone });
        }
    }
    return spans;
}

function commentSpans(lines: string[], kind: Kind): CommentSpan[] {
    const spans: CommentSpan[] = [];
    if (kind === 'script') {
        return scriptSpans(lines);
    }
    if (kind === 'slash') {
        commentLines(lines.join('\n')).forEach((comments, index) => {
            const line = lines[index] ?? '';
            for (const { column, text } of comments) {
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

export function commentParagraphs(lines: string[], kind: Kind): ParagraphLine[][] {
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
        current.push({ number, column: start, line: lines[number - 1] ?? '', alone });
    }
    flush();
    return paragraphs;
}
