/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <optional>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "Scanner.h"

namespace v3d::script::internal {

/**
 * Reads TypeScript just far enough to find its types, and makes each a blank.
 *
 * It is a parser of statements and expressions that keeps nothing of what it reads. Where an
 * annotation, a type argument or a declaration of types begins, it reads the type to its end,
 * and records the run as erased. A run is blanked with spaces, and its line breaks are kept,
 * so every line and column of the JavaScript left is where it was.
 *
 * Some TypeScript can be read two ways. A parenthesis may open an expression or an arrow
 * function's parameters, and a < may be a comparison or open a call's type arguments. The
 * parser tries one, and goes back to try the other when the first does not read. Syntax with
 * no JavaScript to keep when its types go, such as an enum, is refused.
 **/
class Eraser final {
 public:
    explicit Eraser(std::string_view source);

    /**
     * @return whether a word is a modifier TypeScript adds to a class member or a parameter,
     *         which is erased
     **/
    static bool modifier(const Lexeme& token);

    /**
     * Read the whole source.
     * @return the JavaScript left, or an empty text when something was refused
     **/
    std::string run();

    /**
     * @return where reading stopped and why, when run() refused the source
     **/
    std::size_t where() const noexcept;
    const std::string& why() const noexcept;

 private:
    /**
     * Thrown where the source does not read as the parser expects. A try catches it and goes
     * back. Anywhere else, it stops the run.
     **/
    struct Mismatch final {
        std::size_t at;
    };

    /**
     * Thrown where the source holds syntax that cannot be erased. No try catches it.
     **/
    struct Refusal final {
        std::size_t at;
        std::string message;
    };

    /**
     * What an expression reads up to and how.
     **/
    struct Flags final {
        bool comma = true;      /**< a comma joins two expressions rather than ending one **/
        bool in = true;         /**< in is an operator, rather than ending a for statement's head **/
        bool returned = true;   /**< an arrow function may have a return type, which is not so in a conditional's first branch **/
        bool heritage = false;  /**< type arguments may come before a {, as in extends Base<T> { **/
    };

    /**
     * A run of the source to blank, as offsets [begin, end). Its first and last characters
     * become first and last, which are spaces unless something has to stand there instead.
     **/
    struct Run final {
        std::size_t begin;
        std::size_t end;
        char first = ' ';
        char last = ' ';
    };

    /**
     * One level of nesting, counted while it lives. A source nested deeper than NESTING is
     * refused rather than read, so that reading it cannot run out of stack.
     **/
    class Level final {
     public:
        explicit Level(Eraser* eraser);
        ~Level();
        Level(const Level&) = delete;
        Level& operator=(const Level&) = delete;

     private:
        Eraser* eraser_;
    };

    /**
     * How deep statements, expressions and types may nest.
     **/
    static constexpr std::size_t NESTING = 256;

    /**
     * What may follow an operation: more of the same operand, as after a call, another operand,
     * as after a binary operator, or the end of the expression.
     **/
    enum class Next : std::uint8_t { Postfix, Operand, End };

    /**
     * Where the parser is, so that a try can go back to it.
     **/
    struct Mark final {
        std::size_t position;
        Lexeme token;
        Lexeme previous;
        std::size_t erased;
    };

    // tokens
    void advance();
    bool at(std::string_view written) const noexcept;
    bool sameLine() const noexcept;
    void expect(std::string_view written);
    Lexeme peek();
    Mark mark() const;
    void back(const Mark& to);
    [[noreturn]] void mismatch() const;
    [[noreturn]] void refuse(const std::string& message) const;
    [[noreturn]] static void refuse(const std::string& message, std::size_t at);

    // what is erased
    void erase(std::size_t begin, std::size_t end, char last = ' ');
    void eraseFrom(std::size_t begin);

    // statements
    void statements();
    void statement();
    bool declaration();
    bool control();
    bool jump();
    bool declarationOfTypes(std::size_t start);
    void interfaceDeclaration(std::size_t start);
    bool typeAlias(std::size_t start);
    void block();
    void terminate();
    void declarations(const Flags& flags);
    void binding();
    void function(bool declaration, std::size_t start);
    void signature(bool constructor);
    void parameters(bool constructor);
    bool parameter(bool constructor);
    void classDeclaration();
    void member();
    bool modifiers(std::size_t start);
    bool indexSignature(std::size_t start);
    void field();
    void forStatement();
    void switchStatement();
    void tryStatement();
    void ambient(std::size_t start);
    void moduleStatement();

    // expressions
    void expression(const Flags& flags);
    bool operand(const Flags& flags);
    bool prefixes();
    void punctuated(const Flags& flags);
    void genericArrow(const Flags& flags);
    void named(const Flags& flags);
    void asyncArrow(const Flags& flags);
    bool operation(const Flags& flags);
    Next namedOperation(const Flags& flags);
    Next punctuatedOperation(const Flags& flags);
    bool postfix();
    bool arrow(const Flags& flags);
    void arrowBody(const Flags& flags);
    void parenthesised();
    void arguments();
    void arrayLiteral();
    void objectLiteral();
    void propertyName();
    void templateLiteral(bool type);
    bool typeArguments(bool heritage);

    // types
    void type(bool conditional = true);
    void unionType();
    void primaryType();
    bool typePrefixes();
    bool namedType();
    void typeParameters();
    void balanced();
    std::optional<std::string_view> opens(bool angled) const;
    void typeList();
    void typeOnlyStatement(std::size_t start);
    void specifiers();

    std::string_view source_;
    Scanner scanner_;
    Lexeme token_;
    Lexeme previous_;
    // the runs to blank, in the order they were found
    std::vector<Run> erased_;
    std::size_t where_ = 0;
    std::string why_;
    std::size_t depth_ = 0;
    // where an arrow function was tried and did not read, and whether a return type was allowed,
    // so that going back over the place does not try it again
    std::set<std::pair<std::size_t, bool>> notArrows_;
};

};  // namespace v3d::script::internal
