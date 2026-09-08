/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../SLLexer.h"

namespace {

typedef v3d::render::offline::SLToken Token;

std::vector<Token> lex(const std::string & source, std::string * error = nullptr) {
    std::istringstream stream(source);
    v3d::render::offline::SLLexer lexer(stream);
    std::vector<Token> tokens;
    for (;;) {
        Token token = lexer.next();
        if (token.kind() == Token::Kind::END) {
            break;
        }
        tokens.push_back(token);
    }
    if (error) {
        *error = lexer.error();
    }
    return tokens;
}

};  // namespace

/**
 * Every kind, in one shader-shaped line: a keyword, an identifier, a number, a string, an
 * operator and the punctuation that groups them.
 **/
BOOST_AUTO_TEST_CASE(sllexer_kinds_test) {
    std::string error;
    std::vector<Token> tokens = lex("surface plastic(float Ks = 0.5; string space = \"world\";) { }", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(tokens.size(), 16u);
    BOOST_CHECK(tokens[0].kind() == Token::Kind::KEYWORD);
    BOOST_CHECK_EQUAL(tokens[0].text(), "surface");
    BOOST_CHECK(tokens[1].kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[1].text(), "plastic");
    BOOST_CHECK(tokens[2].kind() == Token::Kind::PUNCTUATION);
    BOOST_CHECK_EQUAL(tokens[2].text(), "(");
    BOOST_CHECK(tokens[3].kind() == Token::Kind::KEYWORD);
    BOOST_CHECK_EQUAL(tokens[3].text(), "float");
    BOOST_CHECK(tokens[5].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[5].text(), "=");
    BOOST_CHECK(tokens[6].kind() == Token::Kind::NUMBER);
    BOOST_CHECK_EQUAL(tokens[6].value(), 0.5f);
    BOOST_CHECK(tokens[11].kind() == Token::Kind::STRING);
    BOOST_CHECK_EQUAL(tokens[11].text(), "world");
    BOOST_CHECK(tokens[14].kind() == Token::Kind::PUNCTUATION);
    BOOST_CHECK_EQUAL(tokens[14].text(), "{");
}

/**
 * A keyword is a closed set. A name outside it is an identifier however much it looks like
 * part of the language, which is what lets a shader declare a variable called "output" or
 * write its own "noise".
 **/
BOOST_AUTO_TEST_CASE(sllexer_keyword_set_test) {
    std::vector<Token> tokens = lex("varying normal Nf; output noise diffuse mix");

    BOOST_REQUIRE_EQUAL(tokens.size(), 8u);
    BOOST_CHECK(tokens[0].kind() == Token::Kind::KEYWORD);
    BOOST_CHECK(tokens[1].kind() == Token::Kind::KEYWORD);
    BOOST_CHECK(tokens[2].kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[2].text(), "Nf");
    BOOST_CHECK(tokens[4].kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[4].text(), "output");
    BOOST_CHECK(tokens[5].kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK(tokens[6].kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK(tokens[7].kind() == Token::Kind::IDENTIFIER);
}

/**
 * The three lighting constructs are keywords rather than calls, because they take a body.
 **/
BOOST_AUTO_TEST_CASE(sllexer_lighting_keywords_test) {
    std::vector<Token> tokens = lex("illuminance illuminate solar");

    BOOST_REQUIRE_EQUAL(tokens.size(), 3u);
    for (const Token & token : tokens) {
        BOOST_CHECK(token.kind() == Token::Kind::KEYWORD);
    }
}

/**
 * SL has no integer type, so every literal is a float however it was written - with a
 * leading '.', a trailing one, or an exponent.
 **/
BOOST_AUTO_TEST_CASE(sllexer_number_test) {
    std::vector<Token> tokens = lex("1 1.0 .5 2. 1e3 1.5E-2");

    BOOST_REQUIRE_EQUAL(tokens.size(), 6u);
    for (const Token & token : tokens) {
        BOOST_CHECK(token.kind() == Token::Kind::NUMBER);
    }
    BOOST_CHECK_EQUAL(tokens[0].value(), 1.0f);
    BOOST_CHECK_EQUAL(tokens[1].value(), 1.0f);
    BOOST_CHECK_EQUAL(tokens[2].value(), 0.5f);
    BOOST_CHECK_EQUAL(tokens[3].value(), 2.0f);
    BOOST_CHECK_EQUAL(tokens[4].value(), 1000.0f);
    BOOST_CHECK_CLOSE(tokens[5].value(), 0.015f, 0.01f);
}

/**
 * A number carries no sign. A '-' is the subtraction or the negation, so "a-1" is three
 * tokens - which is the difference between a lexer for an expression language and RIB's.
 **/
BOOST_AUTO_TEST_CASE(sllexer_unsigned_number_test) {
    std::vector<Token> tokens = lex("a-1");

    BOOST_REQUIRE_EQUAL(tokens.size(), 3u);
    BOOST_CHECK(tokens[0].kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK(tokens[1].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text(), "-");
    BOOST_CHECK(tokens[2].kind() == Token::Kind::NUMBER);
    BOOST_CHECK_EQUAL(tokens[2].value(), 1.0f);
}

/**
 * A string carries what is between the quotes, spaces included, with the escapes resolved.
 * In SL it names a coordinate space, a texture or a message and has almost nothing done to
 * it, which is why step 5 may require it to be uniform.
 **/
BOOST_AUTO_TEST_CASE(sllexer_string_test) {
    std::vector<Token> tokens = lex("\"two words\" \"a \\\"quote\\\" and a \\\\\" \"\\101\"");

    BOOST_REQUIRE_EQUAL(tokens.size(), 3u);
    BOOST_CHECK(tokens[0].kind() == Token::Kind::STRING);
    BOOST_CHECK_EQUAL(tokens[0].text(), "two words");
    BOOST_CHECK_EQUAL(tokens[1].text(), "a \"quote\" and a \\");
    // \101 is octal for 'A'
    BOOST_CHECK_EQUAL(tokens[2].text(), "A");
}

/**
 * The one and two character operators, longest match first: a '<' is not a '<=' until the
 * '=' is seen.
 **/
BOOST_AUTO_TEST_CASE(sllexer_operator_test) {
    std::vector<Token> tokens = lex("+ - * / = += -= *= /= == != < <= > >= && || ! ? : ^ .");

    BOOST_REQUIRE_EQUAL(tokens.size(), 22u);
    const char* const expected[] = {
        "+", "-", "*", "/", "=", "+=", "-=", "*=", "/=", "==", "!=",
        "<", "<=", ">", ">=", "&&", "||", "!", "?", ":", "^", "."
    };
    for (std::size_t i = 0; i < tokens.size(); i++) {
        BOOST_CHECK(tokens[i].kind() == Token::Kind::OPERATOR);
        BOOST_CHECK_EQUAL(tokens[i].text(), expected[i]);
    }
}

/**
 * '.' is SL's dot product and '^' its cross, and neither is what a reader coming from
 * another language expects. A '.' that a digit follows is a number instead.
 **/
BOOST_AUTO_TEST_CASE(sllexer_dot_test) {
    std::vector<Token> tokens = lex("a . b ^ c + .5");

    BOOST_REQUIRE_EQUAL(tokens.size(), 7u);
    BOOST_CHECK(tokens[1].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text(), ".");
    BOOST_CHECK(tokens[3].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[3].text(), "^");
    BOOST_CHECK(tokens[6].kind() == Token::Kind::NUMBER);
    BOOST_CHECK_EQUAL(tokens[6].value(), 0.5f);
}

/**
 * A '/' is a division unless what follows it opens a comment, which is a decision that
 * takes the character after it.
 **/
BOOST_AUTO_TEST_CASE(sllexer_comment_test) {
    std::string error;
    std::vector<Token> tokens = lex(
        "a / b  // a line comment\n"
        "/* a block one\n"
        "   spanning lines */ c\n"
        "d /", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(tokens.size(), 6u);
    BOOST_CHECK_EQUAL(tokens[0].text(), "a");
    BOOST_CHECK(tokens[1].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[1].text(), "/");
    BOOST_CHECK_EQUAL(tokens[2].text(), "b");
    BOOST_CHECK_EQUAL(tokens[3].text(), "c");
    BOOST_CHECK_EQUAL(tokens[4].text(), "d");
    // a '/' at the very end of the stream is still a division, which is the case the
    // stream's own putback cannot answer
    BOOST_CHECK(tokens[5].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[5].text(), "/");
}

/**
 * A block comment does not nest, and a "//" inside one is as much a part of it as anything
 * else - so the first close ends it and what follows is code again. The second close is
 * then a multiply and a divide, which is what a language without nesting does with one.
 **/
BOOST_AUTO_TEST_CASE(sllexer_block_comment_is_flat_test) {
    std::string error;
    std::vector<Token> tokens = lex("a /* /* // still a comment */ b */ c", &error);

    BOOST_CHECK_EQUAL(error, "");
    BOOST_REQUIRE_EQUAL(tokens.size(), 5u);
    BOOST_CHECK_EQUAL(tokens[0].text(), "a");
    BOOST_CHECK_EQUAL(tokens[1].text(), "b");
    BOOST_CHECK(tokens[2].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[2].text(), "*");
    BOOST_CHECK(tokens[3].kind() == Token::Kind::OPERATOR);
    BOOST_CHECK_EQUAL(tokens[3].text(), "/");
    BOOST_CHECK_EQUAL(tokens[4].text(), "c");
}

/**
 * Every token says where it started, counting from one, because a diagnostic that does not
 * is most of the cost of a diagnostic.
 **/
BOOST_AUTO_TEST_CASE(sllexer_position_test) {
    std::vector<Token> tokens = lex("surface matte(\n    float Ka = 1;\n)");

    BOOST_REQUIRE(tokens.size() > 5u);
    BOOST_CHECK_EQUAL(tokens[0].line(), 1u);
    BOOST_CHECK_EQUAL(tokens[0].column(), 1u);
    BOOST_CHECK_EQUAL(tokens[1].column(), 9u);
    // the first token of the second line, indented four
    BOOST_CHECK_EQUAL(tokens[3].line(), 2u);
    BOOST_CHECK_EQUAL(tokens[3].column(), 5u);
}

BOOST_AUTO_TEST_CASE(sllexer_unterminated_string_test) {
    std::string error;
    std::vector<Token> tokens = lex("string space = \"world;\n", &error);

    BOOST_CHECK(tokens.size() == 3u);
    BOOST_CHECK_EQUAL(error, "unterminated string starting at line 1, column 16");
}

BOOST_AUTO_TEST_CASE(sllexer_unterminated_block_comment_test) {
    std::string error;
    std::vector<Token> tokens = lex("a\n  /* opened and never closed\nb", &error);

    BOOST_CHECK_EQUAL(tokens.size(), 1u);
    BOOST_CHECK_EQUAL(error, "unterminated block comment starting at line 2, column 3");
}

/**
 * A shader that needs cpp is rejected by name rather than mis-parsed. A '#' has no other
 * meaning in SL, so a scene that names such a shader learns what is missing rather than
 * being told its file is malformed.
 **/
BOOST_AUTO_TEST_CASE(sllexer_preprocessor_test) {
    std::string error;
    std::vector<Token> tokens = lex("#include \"common.h\"\nsurface matte() {}", &error);

    BOOST_CHECK_EQUAL(tokens.size(), 0u);
    BOOST_CHECK_EQUAL(error, "the C preprocessor is not run, so a '#' directive cannot be read at line 1, column 1");
}

/**
 * A single '&' is nothing in this language, and saying so beats reporting the identifier
 * after it as unexpected.
 **/
BOOST_AUTO_TEST_CASE(sllexer_half_an_operator_test) {
    std::string error;
    lex("a & b", &error);

    BOOST_CHECK_EQUAL(error, "'&' is not an operator - '&&' is at line 1, column 3");
}

/**
 * An error stops the stream: a lexer with one set yields nothing but END, so a parser above
 * it cannot mistake the tail of a broken file for a shorter shader.
 **/
BOOST_AUTO_TEST_CASE(sllexer_error_ends_the_stream_test) {
    std::istringstream stream("a @ b");
    v3d::render::offline::SLLexer lexer(stream);

    BOOST_CHECK(lexer.next().kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK(lexer.next().kind() == Token::Kind::END);
    BOOST_CHECK(lexer.next().kind() == Token::Kind::END);
    BOOST_CHECK_EQUAL(lexer.error(), "unexpected character '@' at line 1, column 3");
}

/**
 * peek() leaves the token for next() rather than consuming it, which is what a recursive
 * descent parser reads the grammar with.
 **/
BOOST_AUTO_TEST_CASE(sllexer_peek_test) {
    std::istringstream stream("surface matte");
    v3d::render::offline::SLLexer lexer(stream);

    BOOST_CHECK_EQUAL(lexer.peek().text(), "surface");
    BOOST_CHECK_EQUAL(lexer.peek().text(), "surface");
    BOOST_CHECK_EQUAL(lexer.next().text(), "surface");
    BOOST_CHECK_EQUAL(lexer.next().text(), "matte");
    BOOST_CHECK(lexer.peek().kind() == Token::Kind::END);
}
