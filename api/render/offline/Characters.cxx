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
    int character = END;
    if (pushed_) {
        character = pushback_;
        pushed_ = false;
    } else {
        character = stream_.get();
    }
    if (character == '\n') {
        line_++;
        column_ = 0;
    } else if (character != END) {
        column_++;
    }
    return character;
}

int Characters::look() {
    if (pushed_) {
        return pushback_;
    }
    const int character = stream_.peek();
    return stream_.good() ? character : END;
}

void Characters::unget(int character) {
    pushback_ = character;
    pushed_ = true;
    if (character != END) {
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
        const int character = get();
        if (character == END || (lineEnds && character == '\n')) {
            return false;
        }
        if (character == '"') {
            return true;
        }
        if (character != '\\') {
            *text += static_cast<char>(character);
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
                        value = (value * 8) + (get() - '0');
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

bool Characters::digit(int character) {
    return character >= '0' && character <= '9';
}

bool Characters::octal(int character) {
    return character >= '0' && character <= '7';
}

bool Characters::alpha(int character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_';
}

bool Characters::space(int character) {
    return character == ' ' || character == '\t' || character == '\r' || character == '\n' || character == '\f' || character == '\v';
}

std::string Characters::position(unsigned int line, unsigned int column) {
    return " at line " + std::to_string(line) + ", column " + std::to_string(column);
}

};  // namespace v3d::render::offline
