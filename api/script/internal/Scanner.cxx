/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Scanner.h"

#include <array>
#include <cstddef>
#include <string_view>

namespace v3d::script::internal {

namespace {

/**
 * The punctuators of more than one character, longest first so the first that matches is the
 * longest. None begins with >, which is always read alone.
 **/
constexpr std::array<std::string_view, 28> LONG{ "...", "===", "!==", "**=", "<<=", "&&=", "||=", "??=", "=>", "==", "!=", "<=",
"<<", "**", "++", "--", "&&", "||", "??", "?.", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=" };

/**
 * @return whether a byte can begin a name. A byte of a multibyte character can, so a name may
 *         hold any letter beyond ascii
 **/
bool starts(char letter) noexcept {
    const auto byte = static_cast<unsigned char>(letter);
    return (letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z') || letter == '_' || letter == '$' || letter == '\\' || byte >= 0x80;
}

/**
 * @return whether a byte can continue a name
 **/
bool continues(char letter) noexcept {
    return starts(letter) || (letter >= '0' && letter <= '9');
}

/**
 * @return whether a byte is a decimal digit
 **/
bool digit(char letter) noexcept {
    return letter >= '0' && letter <= '9';
}

};  // namespace

/**
 **/
bool Lexeme::is(std::string_view written) const noexcept {
    return (kind == Token::Punctuator || kind == Token::Identifier) && text == written;
}

/**
 **/
Scanner::Scanner(std::string_view source) :
    source_(source) {
    // a first line that opens with #! names an interpreter, and is not script
    if (source_.starts_with("#!")) {
        while (position_ < source_.size() && source_[position_] != '\n') {
            ++position_;
        }
    }
}

/**
 **/
Lexeme Scanner::next() {
    bool unclosed = false;
    const bool newline = skip(&unclosed);
    const std::size_t begin = position_;
    if (unclosed) {
        return made(Token::Unclosed, begin, newline);
    }
    if (position_ >= source_.size()) {
        return made(Token::End, begin, newline);
    }
    const char first = source_[position_];
    if (starts(first)) {
        position_ = name(position_);
        return made(Token::Identifier, begin, newline);
    }
    if (first == '#' && starts(at(position_ + 1))) {
        position_ = name(position_ + 1);
        return made(Token::Private, begin, newline);
    }
    if (digit(first) || (first == '.' && digit(at(position_ + 1)))) {
        position_ = number(position_);
        return made(Token::Number, begin, newline);
    }
    if (first == '"' || first == '\'') {
        return quoted(begin, newline);
    }
    if (first == '`') {
        return templated(begin, true, newline);
    }
    position_ = punctuator(position_);
    return made(Token::Punctuator, begin, newline);
}

/**
 **/
Lexeme Scanner::quoted(std::size_t begin, bool newline) {
    const char quote = source_[begin];
    position_ = begin + 1;
    while (position_ < source_.size() && source_[position_] != quote && source_[position_] != '\n') {
        position_ += source_[position_] == '\\' ? 2 : 1;
    }
    if (position_ >= source_.size() || source_[position_] != quote) {
        return made(Token::Unclosed, begin, newline);
    }
    ++position_;
    return made(Token::String, begin, newline);
}

/**
 **/
std::size_t Scanner::name(std::size_t at) const {
    while (at < source_.size() && continues(source_[at])) {
        ++at;
    }
    return at;
}

/**
 **/
Lexeme Scanner::regex(const Lexeme& slash) {
    position_ = slash.begin + 1;
    bool inClass = false;
    while (position_ < source_.size() && source_[position_] != '\n') {
        const char letter = source_[position_];
        if (letter == '\\') {
            position_ += 2;
            continue;
        }
        ++position_;
        if (letter == '[') {
            inClass = true;
        } else if (letter == ']') {
            inClass = false;
        } else if (letter == '/' && !inClass) {
            while (position_ < source_.size() && continues(source_[position_])) {
                ++position_;
            }
            return made(Token::Regex, slash.begin, slash.newline);
        }
    }
    return made(Token::Unclosed, slash.begin, slash.newline);
}

/**
 **/
Lexeme Scanner::resume(const Lexeme& brace) {
    return templated(brace.begin, false, brace.newline);
}

/**
 **/
std::size_t Scanner::position() const noexcept {
    return position_;
}

/**
 **/
void Scanner::position(std::size_t offset) noexcept {
    position_ = offset;
}

/**
 **/
bool Scanner::skip(bool* unclosed) {
    bool newline = false;
    while (position_ < source_.size()) {
        const char letter = source_[position_];
        if (letter == '\n' || letter == '\r') {
            newline = true;
            ++position_;
        } else if (letter == ' ' || letter == '\t' || letter == '\f' || letter == '\v') {
            ++position_;
        } else if (source_.substr(position_, 2) == "//") {
            while (position_ < source_.size() && source_[position_] != '\n') {
                ++position_;
            }
        } else if (source_.substr(position_, 2) == "/*") {
            const std::size_t end = source_.find("*/", position_ + 2);
            if (end == std::string_view::npos) {
                // the comment is reported where it opens
                *unclosed = true;
                return newline;
            }
            newline = newline || source_.substr(position_, end - position_).find('\n') != std::string_view::npos;
            position_ = end + 2;
        } else if (source_.substr(position_).starts_with("\xC2\xA0")) {
            // a no-break space is white space, and so is a byte order mark
            position_ += 2;
        } else if (source_.substr(position_).starts_with("\xEF\xBB\xBF")) {
            position_ += 3;
        } else {
            break;
        }
    }
    return newline;
}

/**
 **/
Lexeme Scanner::templated(std::size_t begin, bool head, bool newline) {
    position_ = begin + 1;
    while (position_ < source_.size()) {
        const char letter = source_[position_];
        if (letter == '\\') {
            position_ += 2;
        } else if (letter == '`') {
            ++position_;
            return made(Token::Template, begin, newline);
        } else if (letter == '$' && at(position_ + 1) == '{') {
            position_ += 2;
            return made(head ? Token::TemplateHead : Token::TemplateMiddle, begin, newline);
        } else {
            ++position_;
        }
    }
    position_ = source_.size();
    return made(Token::Unclosed, begin, newline);
}

/**
 **/
Lexeme Scanner::made(Token kind, std::size_t begin, bool newline) const {
    const std::size_t end = position_ > source_.size() ? source_.size() : position_;
    return Lexeme{ .kind = kind, .begin = begin, .end = end, .text = source_.substr(begin, end - begin), .newline = newline };
}

/**
 **/
std::size_t Scanner::number(std::size_t at) const {
    // a number's digits, letters, separators, point and exponent sign run together, and the
    // engine reads the number they make. Only where it ends matters here
    const char second = this->at(at + 1);
    const bool radix = source_[at] == '0' && (second == 'x' || second == 'X' || second == 'b' || second == 'B' || second == 'o' || second == 'O');
    while (at < source_.size()) {
        const char letter = source_[at];
        const char before = source_[at - (at > 0 ? 1 : 0)];
        const bool sign = (letter == '+' || letter == '-') && (before == 'e' || before == 'E');
        if (!continues(letter) && ((letter != '.' && !sign) || radix)) {
            break;
        }
        ++at;
    }
    return at;
}

/**
 **/
std::size_t Scanner::punctuator(std::size_t at) const {
    const std::string_view rest = source_.substr(at);
    // ?. followed by a digit is a ? and a number, as in a?.5:0
    if (rest.starts_with("?.") && digit(this->at(at + 2))) {
        return at + 1;
    }
    for (const std::string_view each : LONG) {
        if (rest.starts_with(each)) {
            return at + each.size();
        }
    }
    return at + 1;
}

/**
 **/
char Scanner::at(std::size_t offset) const noexcept {
    return offset < source_.size() ? source_[offset] : '\0';
}

};  // namespace v3d::script::internal
