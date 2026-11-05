/**
 * Reading the code of a TypeScript or JavaScript file, apart from its comments and literals.
 *
 * The scanner skips strings, template literals and regular expression literals, so a // or /*
 * inside one starts no comment. A template's ${} expressions are read as code. A slash starts a
 * regular expression where an operand is expected: after an operator, an opening bracket, a
 * comma, a keyword such as return, or at the start of the text.
 */

/** What a stretch of a file is. */
export type Kind = 'code' | 'comment' | 'literal';

/** A stretch of a file, from start up to end, and what it is. */
export interface Region {
    kind: Kind;
    start: number;
    end: number;
}

/** The keywords after which a slash starts a regular expression rather than dividing. */
const BEFORE_OPERAND = /^(return|typeof|case|do|else|in|of|new|delete|void|throw|instanceof|yield|await)$/;

/** The end of the regular expression literal starting at a slash, or -1 for a division. */
function regexEnd(text: string, start: number): number {
    const before = text.slice(0, start).trimEnd();
    const word = /[\w$]+$/.exec(before);
    if (word ? !BEFORE_OPERAND.test(word[0]) : /[)\]"'`]$/.test(before)) {
        return -1;
    }
    let inClass = false;
    for (let index = start + 1; index < text.length; index++) {
        const char = text[index];
        if (char === '\n') {
            return -1;
        } else if (char === '\\') {
            index += 1;
        } else if (char === '[') {
            inClass = true;
        } else if (char === ']') {
            inClass = false;
        } else if (char === '/' && !inClass) {
            return index + 1;
        }
    }
    return -1;
}

/**
 * The comments and literals of a file, in order. A comment's region includes its markers. A
 * literal's region holds only the text between its delimiters, and the quotes, backticks and
 * slashes stay code. The text between the regions is code.
 */
export function regions(text: string): Region[] {
    const found: Region[] = [];
    // the brace depth at which each open ${ of a template closes
    const templates: number[] = [];
    let braces = 0;
    let index = 0;
    // the start of the template text being read, or -1 outside a template
    let template = -1;
    while (index < text.length) {
        const char = text[index];
        if (template >= 0) {
            if (char === '\\') {
                index += 2;
            } else if (char === '`') {
                found.push({ kind: 'literal', start: template, end: index });
                template = -1;
                index += 1;
            } else if (text.startsWith('${', index)) {
                found.push({ kind: 'literal', start: template, end: index });
                template = -1;
                templates.push(braces);
                braces += 1;
                index += 2;
            } else {
                index += 1;
            }
            continue;
        }
        if (text.startsWith('//', index)) {
            const end = text.indexOf('\n', index);
            found.push({ kind: 'comment', start: index, end: end < 0 ? text.length : end });
            index = end < 0 ? text.length : end;
        } else if (text.startsWith('/*', index)) {
            const end = text.indexOf('*/', index + 2);
            found.push({ kind: 'comment', start: index, end: end < 0 ? text.length : end + 2 });
            index = end < 0 ? text.length : end + 2;
        } else if (char === '"' || char === '\'') {
            let end = index + 1;
            while (end < text.length && text[end] !== char && text[end] !== '\n') {
                end += text[end] === '\\' ? 2 : 1;
            }
            found.push({ kind: 'literal', start: index + 1, end: Math.min(end, text.length) });
            index = end + 1;
        } else if (char === '`') {
            template = index + 1;
            index += 1;
        } else if (char === '/') {
            const end = regexEnd(text, index);
            if (end < 0) {
                index += 1;
            } else {
                found.push({ kind: 'literal', start: index + 1, end: end - 1 });
                index = end;
            }
        } else {
            if (char === '{') {
                braces += 1;
            } else if (char === '}') {
                braces -= 1;
                if (templates.length > 0 && templates[templates.length - 1] === braces) {
                    templates.pop();
                    template = index + 1;
                }
            }
            index += 1;
        }
    }
    if (template >= 0) {
        found.push({ kind: 'literal', start: template, end: text.length });
    }
    return found.filter((region) => region.end > region.start);
}

/**
 * The text with every comment and the inside of every literal replaced by spaces. Line breaks
 * are kept, so an offset or a line number in the result is the same in the text.
 */
export function codeOnly(text: string): string {
    const units = text.split('');
    for (const { start, end } of regions(text)) {
        for (let at = start; at < end; at++) {
            if (units[at] !== '\n') {
                units[at] = ' ';
            }
        }
    }
    return units.join('');
}
