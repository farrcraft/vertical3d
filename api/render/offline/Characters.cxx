/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Characters.h"

#include <istream>
#include <string>

namespace v3d::render::offline {

Characters::Characters(std::istream & stream) : stream_(stream) {
}

int Characters::get() {
    int c = END;
    if (pushed_) {
        c = pushback_;
        pushed_ = false;
    } else {
        c = stream_.get();
    }
    if (c == '\n') {
        line_++;
        column_ = 0;
    } else if (c != END) {
        column_++;
    }
    return c;
}

int Characters::look() {
    if (pushed_) {
        return pushback_;
    }
    const int c = stream_.peek();
    return stream_.good() ? c : END;
}

void Characters::unget(int c) {
    pushback_ = c;
    pushed_ = true;
    if (c != END) {
        column_--;
    }
}

unsigned int Characters::line() const {
    return line_;
}

unsigned int Characters::column() const {
    return column_;
}

bool Characters::quoted(bool lineEnds, std::string* text) {
    for (;;) {
        const int c = get();
        if (c == END || (lineEnds && c == '\n')) {
            return false;
        }
        if (c == '"') {
            return true;
        }
        if (c != '\\') {
            *text += static_cast<char>(c);
            continue;
        }
        const int escape = get();
        switch (escape) {
            case 'n': *text += '\n'; break;
            case 't': *text += '\t'; break;
            case 'r': *text += '\r'; break;
            case 'b': *text += '\b'; break;
            case 'f': *text += '\f'; break;
            case '\\': *text += '\\'; break;
            case '"': *text += '"'; break;
            case END:
                return false;
            default:
                if (octal(escape)) {
                    int value = escape - '0';
                    for (int i = 1; i < 3 && octal(look()); i++) {
                        value = value * 8 + (get() - '0');
                    }
                    *text += static_cast<char>(value);
                } else {
                    // an escape the standard does not define is the character itself
                    *text += static_cast<char>(escape);
                }
                break;
        }
    }
}

bool Characters::digit(int c) {
    return c >= '0' && c <= '9';
}

bool Characters::octal(int c) {
    return c >= '0' && c <= '7';
}

bool Characters::alpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool Characters::space(int c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

std::string Characters::position(unsigned int line, unsigned int column) {
    return " at line " + std::to_string(line) + ", column " + std::to_string(column);
}

};  // namespace v3d::render::offline
