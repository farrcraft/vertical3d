/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/integrate/Event.h>
#include <moya/libmoya/lpe/Pattern.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace v3d::moya::lpe::internal {

/** A recursive descent through an expression, which throws Stopped where it cannot go on. **/
class Reader final {
 public:
    /** A reading that stops, at a place and for a reason. **/
    struct Stopped {
        std::size_t place { 0 };
        std::string reason;
    };


    Reader(const std::string & text, const std::vector<std::string> & labels) :
        text_(text), labels_(labels), symbols_(kernel::integrate::symbolsOf(static_cast<std::uint32_t>(labels.size()))) {}

    Pattern whole() {
        skip();
        if (at_ == text_.size()) {
            stop("an expression holds at least one event");
        }
        Pattern out = choice();
        if (at_ != text_.size()) {
            stop("an unmatched ')'");
        }
        return out;
    }

 private:
    /** The letters of the events' types, and of their scatterings. **/
    static constexpr std::string_view kTypes = "CRTVLOB";
    static constexpr std::string_view kScatterings = "DGSs";

    /** The most places an event holds: its type, its scattering and its label. **/
    static constexpr std::size_t kPlaces = 3;

    /** The largest count a repetition is read with, past which reading it stops. **/
    static constexpr std::uint32_t kLargestCount = 1000000;

    /** The base a repetition's count is written in. **/
    static constexpr std::uint32_t kDecimal = 10;

    /**
     * What a symbol stands for, as an index into each place's values. A type is its letter's place
     * among kTypes. A scattering is zero for none, or one more than its letter's place among
     * kScatterings. A label is zero for none, or one more than its label.
     **/
    struct Described {
        std::size_t type { 0 };
        std::size_t scattering { 0 };
        std::size_t label { 0 };
    };

    /** What a symbol stands for. **/
    static Described describe(std::uint32_t symbol) {
        if (symbol == kernel::integrate::kCameraSymbol) {
            return Described { .type = kTypes.find('C') };
        }
        if (symbol < kernel::integrate::kTransmissionSymbol) {
            return Described { .type = kTypes.find('R'), .scattering = 1U + symbol - kernel::integrate::kReflectionSymbol };
        }
        if (symbol < kernel::integrate::kObjectSymbol) {
            return Described { .type = kTypes.find('T'), .scattering = 1U + symbol - kernel::integrate::kTransmissionSymbol };
        }
        if (symbol == kernel::integrate::kObjectSymbol) {
            return Described { .type = kTypes.find('O') };
        }
        if (symbol == kernel::integrate::kLightSymbol) {
            return Described { .type = kTypes.find('L') };
        }
        return Described { .type = kTypes.find('L'), .label = 1U + symbol - kernel::integrate::kLabelled };
    }

    const std::string & text_;
    const std::vector<std::string> & labels_;
    std::uint32_t symbols_;
    std::size_t at_ { 0 };

    [[noreturn]] void stop(const std::string & reason) const {
        throw Stopped { .place = at_, .reason = reason };
    }

    void skip() {
        while (at_ < text_.size() && text_[at_] == ' ') {
            ++at_;
        }
    }

    /** The next character past any spaces, or none at the end. **/
    char peek() {
        skip();
        return at_ < text_.size() ? text_[at_] : '\0';
    }

    void expect(char wanted) {
        if (peek() != wanted) {
            stop(std::string("expected '") + wanted + "'");
        }
        ++at_;
    }

    Pattern choice() {
        Pattern first = sequence();
        if (peek() != '|') {
            return first;
        }
        Pattern out { .kind = Pattern::Kind::Choice };
        out.parts.push_back(std::move(first));
        while (peek() == '|') {
            ++at_;
            out.parts.push_back(sequence());
        }
        return out;
    }

    Pattern sequence() {
        Pattern out { .kind = Pattern::Kind::Sequence };
        for (char next = peek(); next != '\0' && next != '|' && next != ')'; next = peek()) {
            out.parts.push_back(repeated());
        }
        return out.parts.size() == 1U ? std::move(out.parts.front()) : out;
    }

    Pattern repeated() {
        Pattern out = atom();
        for (char next = peek(); next == '*' || next == '+' || next == '?' || next == '{'; next = peek()) {
            Pattern repeat { .kind = Pattern::Kind::Repeat };
            ++at_;
            if (next == '{') {
                counts(repeat);
            } else {
                repeat.least = next == '+' ? 1U : 0U;
                repeat.most = next == '?' ? 1U : kUnbounded;
            }
            repeat.parts.push_back(std::move(out));
            out = std::move(repeat);
        }
        return out;
    }

    /** The counts of {n}, {n,} or {n,m}, past its opening brace. **/
    void counts(Pattern & repeat) {
        repeat.least = count();
        repeat.most = repeat.least;
        if (peek() == ',') {
            ++at_;
            repeat.most = peek() == '}' ? kUnbounded : count();
        }
        if (repeat.most != kUnbounded && repeat.most < repeat.least) {
            stop("a repetition's most is below its least");
        }
        expect('}');
    }

    std::uint32_t count() {
        skip();
        const std::size_t from = at_;
        std::uint32_t out = 0;
        while (at_ < text_.size() && text_[at_] >= '0' && text_[at_] <= '9' && out <= kLargestCount) {
            out = (out * kDecimal) + static_cast<std::uint32_t>(text_[at_] - '0');
            ++at_;
        }
        if (at_ == from) {
            stop("a repetition's count is a number");
        }
        if (out > kMostRepeats) {
            at_ = from;
            stop("a repetition's count is at most " + std::to_string(kMostRepeats));
        }
        return out;
    }

    Pattern atom() {
        const char next = peek();
        if (next == '(') {
            ++at_;
            Pattern out = choice();
            expect(')');
            return out;
        }
        if (next == '[') {
            ++at_;
            return events(set());
        }
        return events(event());
    }

    /** The events of a set in brackets, past its opening bracket. **/
    std::vector<bool> set() {
        const bool negated = peek() == '^';
        at_ += negated ? 1U : 0U;
        std::vector<bool> out(symbols_, false);
        if (peek() == ']') {
            stop("a set holds at least one event");
        }
        while (peek() != ']') {
            if (peek() == '\0') {
                stop("expected ']'");
            }
            const std::vector<bool> one = event();
            for (std::uint32_t i = 0; i < symbols_; ++i) {
                out[i] = out[i] || one[i];
            }
        }
        ++at_;
        if (negated) {
            out.flip();
        }
        return out;
    }

    static Pattern events(std::vector<bool> symbols) {
        return Pattern { .kind = Pattern::Kind::Events, .events = std::move(symbols) };
    }

    /** One event: a letter, a quoted label, '.', or one written in angle brackets. **/
    std::vector<bool> event() {
        const char next = peek();
        std::vector<std::vector<bool>> places = anyPlaces();
        if (next == '<') {
            ++at_;
            for (std::size_t place = 0; peek() != '>'; ++place) {
                if (place == kPlaces) {
                    stop("an event holds a type, a scattering and one label");
                }
                if (peek() == '\0') {
                    stop("expected '>'");
                }
                places[place] = value(place);
            }
            ++at_;
        } else if (next == '\'') {
            places[2] = value(2);
        } else if (next == '.') {
            ++at_;
        } else if (kTypes.find(next) != std::string_view::npos) {
            places[0] = value(0);
        } else if (next != '\0' && kScatterings.find(next) != std::string_view::npos) {
            places[1] = value(1);
        } else {
            stop(next == '\0' ? "expected an event" : std::string("an event is not written '") + next + "'");
        }
        return symbolsOf(places);
    }

    /** Each place's values, every one of them allowed. **/
    std::vector<std::vector<bool>> anyPlaces() const {
        return { std::vector<bool>(kTypes.size(), true), std::vector<bool>(kScatterings.size() + 1U, true),
            std::vector<bool>(labels_.size() + 1U, true) };
    }

    /** The values a place of an event allows: one, '.' for any, or a set of them. **/
    std::vector<bool> value(std::size_t place) {
        const std::vector<std::vector<bool>> any = anyPlaces();
        if (peek() == '.') {
            ++at_;
            return any[place];
        }
        if (peek() != '[') {
            std::vector<bool> out(any[place].size(), false);
            one(place, out);
            return out;
        }
        ++at_;
        const bool negated = peek() == '^';
        at_ += negated ? 1U : 0U;
        std::vector<bool> out(any[place].size(), false);
        do {
            if (peek() == '\0') {
                stop("expected ']'");
            }
            one(place, out);
        } while (peek() != ']');
        ++at_;
        if (negated) {
            out.flip();
        }
        return out;
    }

    /** Allows one value of a place: a type's or a scattering's letter, or a quoted label. **/
    void one(std::size_t place, std::vector<bool> & out) {
        const char next = peek();
        if (place == 2) {
            const std::size_t found = label();
            if (found != std::string::npos) {
                out[found] = true;
            }
            return;
        }
        const std::string_view letters = place == 0 ? kTypes : kScatterings;
        const std::size_t found = next == '\0' ? std::string_view::npos : letters.find(next);
        if (found == std::string_view::npos) {
            stop(place == 0 ? "a type is one of C, R, T, V, L, O and B" : "a scattering is one of D, G, S and s");
        }
        ++at_;
        out[place == 0 ? found : found + 1U] = true;
    }

    /** A quoted label's value: one more than its place among the labels, or npos for none. **/
    std::size_t label() {
        if (peek() != '\'') {
            stop("a label is quoted, as 'key'");
        }
        const std::size_t close = text_.find('\'', at_ + 1U);
        if (close == std::string::npos) {
            stop("a label's quote is not closed");
        }
        const std::string name = text_.substr(at_ + 1U, close - at_ - 1U);
        at_ = close + 1U;
        for (std::size_t i = 0; i < labels_.size(); ++i) {
            if (labels_[i] == name) {
                return i + 1U;
            }
        }
        return std::string::npos;
    }

    /** The symbols whose type, scattering and label the places allow. **/
    std::vector<bool> symbolsOf(const std::vector<std::vector<bool>> & places) const {
        std::vector<bool> out(symbols_, false);
        for (std::uint32_t symbol = 0; symbol < symbols_; ++symbol) {
            const Described described = describe(symbol);
            out[symbol] = places[0][described.type] && places[1][described.scattering] && places[2][described.label];
        }
        return out;
    }
};

};  // namespace v3d::moya::lpe::internal
