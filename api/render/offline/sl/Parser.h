/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/syntax/Block.h>
#include <api/render/offline/sl/syntax/Expression.h>
#include <api/render/offline/sl/syntax/Lighting.h>
#include <api/render/offline/sl/syntax/Parameter.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/sl/syntax/Statement.h>

#include <iosfwd>
#include <string>
#include <vector>

#include "Lexer.h"
#include "Token.h"
#include "Types.h"

namespace v3d::render::offline::sl {

/**
 * Recursive descent over a shading language source.
 *
 * Recursive descent rather than a generated parser because the grammar is small and the
 * error messages are the reason anyone will read this code: every diagnostic names a line,
 * a column and what was expected, which a table driven parser makes hard to say well.
 *
 * All five shader types parse. The two this phase does not execute - displacement and
 * volume - come back as shaders that answer false to `Shader::supported()`, so a scene
 * carrying one is told what is unsupported rather than what is malformed.
 *
 * **A parse that fails yields no shaders at all**, and error() says why. What a renderer
 * does about that is step 8's answer, not this class's.
 **/
class Parser final {
 public:
    /**
     * The stream is read as the parse runs and has to outlive the call to parse().
     **/
    explicit Parser(std::istream & stream);

    /**
     * Every shader the source declares, in the order it declared them. Empty on failure,
     * and empty for a source that declares none.
     **/
    std::vector<syntax::ShaderPtr> parse();

    /**
     * What went wrong, or empty.
     **/
    const std::string & error() const;

 private:
    /**
     * Thrown by the productions and caught by parse(), which is what keeps a production
     * reading like the grammar rule it implements rather than like a chain of null checks.
     **/
    class Failure final {};

    /**
     * The next token, left for next() to take.
     *
     * The reference is into the lexer's one token slot, so a token that has to outlive the
     * next consumption is copied rather than bound. A lexer error surfaces here rather than
     * as the END it hands back, which would otherwise be reported as a truncated shader.
     **/
    const Token & peek();
    Token next();
    /**
     * Whether the next token is that keyword, operator or punctuation mark, without
     * consuming it.
     **/
    bool at(Token::Kind kind, const std::string & text);
    /**
     * Consume the next token when it is that one, and say whether it was.
     **/
    bool accept(Token::Kind kind, const std::string & text);
    /**
     * Consume the next token, failing when it is not that one.
     **/
    Token expect(Token::Kind kind, const std::string & text);
    /**
     * Consume the next token, failing when it is not of that kind. `what` names what was
     * wanted, for the diagnostic - "an identifier", "a shader name".
     **/
    Token expectKind(Token::Kind kind, const char* what);

    syntax::ShaderPtr parseShader();
    /**
     * A shader's parameter list, whose every parameter carries a required default, or a
     * function's formals, which carry none.
     **/
    void parseParameters(std::vector<syntax::Parameter>* parameters, bool defaults);
    bool parseType(Type* type);
    Storage parseStorage();

    syntax::BlockPtr parseBlock();
    syntax::StatementPtr parseStatement();
    /**
     * The statements that open with a keyword, which is all of them but an assignment and a
     * bare expression.
     **/
    syntax::StatementPtr parseKeywordStatement(const Token & keyword);
    syntax::StatementPtr parseConditional();
    syntax::StatementPtr parseWhile();
    syntax::StatementPtr parseFor();
    /**
     * break, continue or return - the last with an optional value.
     **/
    syntax::StatementPtr parseJump();
    /**
     * A declaration inside a body, where a function definition is not allowed and says so.
     **/
    syntax::StatementPtr parseLocalDeclaration();
    /**
     * The rest of a declaration, from the first name onward.
     *
     * The name is already consumed because a shader body's `float x` and `float f(` are
     * told apart by the token after it, and the lexer looks one token ahead rather than
     * two.
     **/
    syntax::StatementPtr parseDeclaration(Storage storage, Type type, const Token & first);
    /**
     * An assignment or a bare expression, leaving the semicolon for the caller - which is
     * what lets a for loop's head reuse it.
     **/
    syntax::StatementPtr parseSimpleStatement();
    syntax::StatementPtr parseLighting(syntax::Lighting::Construct construct);

    syntax::ExpressionPtr parseExpression();
    syntax::ExpressionPtr parseTernary();
    syntax::ExpressionPtr parseLogicalOr();
    syntax::ExpressionPtr parseLogicalAnd();
    syntax::ExpressionPtr parseEquality();
    syntax::ExpressionPtr parseComparison();
    syntax::ExpressionPtr parseAdditive();
    syntax::ExpressionPtr parseMultiplicative();
    /**
     * '.' and '^' - the dot and cross products - which bind tighter than a multiply.
     **/
    syntax::ExpressionPtr parseProduct();
    syntax::ExpressionPtr parseUnary();
    syntax::ExpressionPtr parsePostfix();
    syntax::ExpressionPtr parsePrimary();
    /**
     * A parenthesised expression or a comma separated list: three elements are a point or
     * a colour and sixteen are a matrix, which is the compiler's check rather than this
     * one's.
     **/
    syntax::ExpressionPtr parseParenthesised();

    Failure fail(const std::string & message, const Token & token);

    Lexer lexer_;
    std::string error_;
};

};  // namespace v3d::render::offline::sl
