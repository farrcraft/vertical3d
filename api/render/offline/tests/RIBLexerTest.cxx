/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "../RIBLexer.h"

namespace {

typedef v3d::render::offline::RIBToken Token;

std::vector<Token> lex(const std::string & source, std::string * error = nullptr) {
    std::istringstream stream(source);
    v3d::render::offline::RIBLexer lexer(stream);
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

BOOST_AUTO_TEST_CASE(riblexer_request_test) {
    std::vector<Token> tokens = lex("Format 640 480 1");

    BOOST_REQUIRE_EQUAL(tokens.size(), 4u);
    BOOST_CHECK(tokens[0].kind() == Token::Kind::IDENTIFIER);
    BOOST_CHECK_EQUAL(tokens[0].text(), "Format");
    BOOST_CHECK(tokens[1].kind() == Token::Kind::NUMBER);
    BOOST_CHECK_EQUAL(tokens[1].value(), 640.0f);
    BOOST_CHECK_EQUAL(tokens[3].value(), 1.0f);
}

/**
 * A string carries what is between the quotes, spaces included, with the escapes resolved.
 **/
BOOST_AUTO_TEST_CASE(riblexer_string_test) {
    std::vector<Token> tokens = lex("\"two words\" \"a \\\"quote\\\" and a \\\\\" \"\\101\"");

    BOOST_REQUIRE_EQUAL(tokens.size(), 3u);
    BOOST_CHECK(tokens[0].kind() == Token::Kind::STRING);
    BOOST_CHECK_EQUAL(tokens[0].text(), "two words");
    BOOST_CHECK_EQUAL(tokens[1].text(), "a \"quote\" and a \\");
    // \101 is octal for 'A'
    BOOST_CHECK_EQUAL(tokens[2].text(), "A");
}

BOOST_AUTO_TEST_CASE(riblexer_unterminated_string_test) {
    std::string error;
    std::vector<Token> tokens = lex("Surface \"plastic", &error);

    BOOST_CHECK_EQUAL(tokens.size(), 1u);
    BOOST_CHECK(error.find("unterminated string") != std::string::npos);
    // and it says where
    BOOST_CHECK(error.find("column 9") != std::string::npos);
}

/**
 * Whitespace is not significant, so an array spans lines - which is what the whitespace split
 * this replaced could not read.
 **/
BOOST_AUTO_TEST_CASE(riblexer_array_across_lines_test) {
    std::vector<Token> tokens = lex("Polygon \"P\" \n[-100. 0. -100.\n 100. 0. 100.]\n");

    BOOST_REQUIRE_EQUAL(tokens.size(), 10u);
    BOOST_CHECK(tokens[2].kind() == Token::Kind::ARRAY_BEGIN);
    BOOST_CHECK_EQUAL(tokens[3].value(), -100.0f);
    BOOST_CHECK(tokens[9].kind() == Token::Kind::ARRAY_END);
}

/**
 * A leading and a trailing point are both legal and both appear in the standard's own example
 * file, as does an exponent.
 **/
BOOST_AUTO_TEST_CASE(riblexer_number_forms_test) {
    std::vector<Token> tokens = lex(".5 5. -1e3 +2.5E-2 1.0e38 0");

    BOOST_REQUIRE_EQUAL(tokens.size(), 6u);
    BOOST_CHECK_EQUAL(tokens[0].value(), 0.5f);
    BOOST_CHECK_EQUAL(tokens[1].value(), 5.0f);
    BOOST_CHECK_EQUAL(tokens[2].value(), -1000.0f);
    BOOST_CHECK_CLOSE(tokens[3].value(), 0.025f, 0.001f);
    BOOST_CHECK_CLOSE(tokens[4].value(), 1.0e38f, 0.001f);
    BOOST_CHECK_EQUAL(tokens[5].value(), 0.0f);
}

/**
 * A comment runs to the end of the line, and a structure comment is a comment.
 **/
BOOST_AUTO_TEST_CASE(riblexer_comment_test) {
    std::vector<Token> tokens = lex("##RenderMan RIB-Structure 1.1\nFormat 32 16 1  #renderer specific\nWorldBegin");

    BOOST_REQUIRE_EQUAL(tokens.size(), 5u);
    BOOST_CHECK_EQUAL(tokens[0].text(), "Format");
    BOOST_CHECK_EQUAL(tokens[4].text(), "WorldBegin");
}

/**
 * The position is the token's first character, counting from one.
 **/
BOOST_AUTO_TEST_CASE(riblexer_position_test) {
    std::vector<Token> tokens = lex("Format 640\n  WorldBegin");

    BOOST_REQUIRE_EQUAL(tokens.size(), 3u);
    BOOST_CHECK_EQUAL(tokens[0].line(), 1u);
    BOOST_CHECK_EQUAL(tokens[0].column(), 1u);
    BOOST_CHECK_EQUAL(tokens[1].column(), 8u);
    BOOST_CHECK_EQUAL(tokens[2].line(), 2u);
    BOOST_CHECK_EQUAL(tokens[2].column(), 3u);
}

/**
 * Both encodings are part of the format and neither is in scope. Parsed as ASCII they become
 * nonsense rather than an error, which is what this refuses to do.
 **/
BOOST_AUTO_TEST_CASE(riblexer_binary_rejected_test) {
    std::string error;
    lex(std::string("\x80\x05" "Format", 8), &error);
    BOOST_CHECK(error.find("binary RIB") != std::string::npos);

    std::string gzipped;
    lex(std::string("\x1f\x8b\x08\x00", 4), &gzipped);
    BOOST_CHECK(gzipped.find("gzipped RIB") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(riblexer_unexpected_character_test) {
    std::string error;
    lex("Format 640 * 480", &error);

    BOOST_CHECK(error.find("unexpected character '*'") != std::string::npos);
}

/**
 * The standard's own example file, which is the widest RIB in the tree: quoted strings,
 * bracketed arrays spanning lines, structure comments, unbracketed parameter values and every
 * number form.
 **/
BOOST_AUTO_TEST_CASE(riblexer_example_file_test) {
    std::ifstream file("data/example.rib");
    BOOST_REQUIRE(file.is_open());

    v3d::render::offline::RIBLexer lexer(file);
    unsigned int identifiers = 0;
    unsigned int count = 0;
    for (Token token = lexer.next(); token.kind() != Token::Kind::END; token = lexer.next()) {
        if (token.kind() == Token::Kind::IDENTIFIER) {
            identifiers++;
        }
        count++;
    }

    BOOST_CHECK_EQUAL(lexer.error(), "");
    BOOST_CHECK_GT(count, 200u);
    // one per request, and the file holds two frames of them
    BOOST_CHECK_GT(identifiers, 40u);
}

/**
 * peek() leaves the token for next(), which is what the parser above reads a request name with
 * before deciding whether it owns what follows.
 **/
BOOST_AUTO_TEST_CASE(riblexer_peek_test) {
    std::istringstream stream("Format 640");
    v3d::render::offline::RIBLexer lexer(stream);

    BOOST_CHECK_EQUAL(lexer.peek().text(), "Format");
    BOOST_CHECK_EQUAL(lexer.peek().text(), "Format");
    BOOST_CHECK_EQUAL(lexer.next().text(), "Format");
    BOOST_CHECK_EQUAL(lexer.next().value(), 640.0f);
    BOOST_CHECK(lexer.next().kind() == Token::Kind::END);
}
