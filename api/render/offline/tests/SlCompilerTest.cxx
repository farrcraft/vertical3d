/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/Symbol.h>
#include <api/render/offline/sl/syntax/Shader.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::sl::Storage Storage;
typedef v3d::render::offline::sl::Symbol Symbol;

/**
 * Parse one shader and compile it, returning the compiler's error. An empty result means the
 * shader compiled cleanly.
 **/
std::string compile(const std::string & source, std::vector<Symbol>* symbols = nullptr) {
    std::istringstream stream(source);
    v3d::render::offline::sl::Parser parser(stream);
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
    if (shaders.size() != 1) {
        return parser.error().empty() ? "no shader" : parser.error();
    }
    v3d::render::offline::sl::Compiler compiler(shaders[0]);
    const bool compiled = compiler.compile();
    if (symbols) {
        *symbols = compiler.symbols();
    }
    if (compiled) {
        return "";
    }
    return compiler.error();
}

/**
 * The storage a named symbol came out as. For a name declared more than once, the last
 * declaration's storage is returned.
 **/
Storage storageOf(const std::vector<Symbol> & symbols, const std::string & name) {
    Storage found = Storage::UNSPECIFIED;
    for (const Symbol & symbol : symbols) {
        if (symbol.name == name) {
            found = symbol.storage;
        }
    }
    return found;
}

const char* const CONSTANT =
"surface constant() {\n"
"    Ci = Cs;\n"
"    Oi = Os;\n"
"}\n";

const char* const MATTE =
"surface matte(float Ka = 1; float Kd = 1;) {\n"
"    normal Nf = faceforward(normalize(N), I);\n"
"    Oi = Os;\n"
"    Ci = Os * Cs * (Ka * ambient() + Kd * diffuse(Nf));\n"
"}\n";

const char* const METAL =
"surface metal(float Ka = 1; float Ks = 1; float roughness = 0.1;) {\n"
"    normal Nf = faceforward(normalize(N), I);\n"
"    vector V = -normalize(I);\n"
"    Oi = Os;\n"
"    Ci = Os * Cs * (Ka * ambient() + Ks * specular(Nf, V, roughness));\n"
"}\n";

const char* const PLASTIC =
"surface plastic(float Ka = 1; float Kd = 0.5; float Ks = 0.5;\n"
"                float roughness = 0.1; color specularcolor = 1;) {\n"
"    normal Nf = faceforward(normalize(N), I);\n"
"    vector V = -normalize(I);\n"
"    Oi = Os;\n"
"    Ci = Os * (Cs * (Ka * ambient() + Kd * diffuse(Nf)) +\n"
"               specularcolor * Ks * specular(Nf, V, roughness));\n"
"}\n";

const char* const AMBIENTLIGHT =
"light ambientlight(float intensity = 1; color lightcolor = 1;) {\n"
"    Cl = intensity * lightcolor;\n"
"}\n";

const char* const DISTANTLIGHT =
"light distantlight(float intensity = 1; color lightcolor = 1;\n"
"                   point from = point \"shader\" (0, 0, 0);\n"
"                   point to = point \"shader\" (0, 0, 1);) {\n"
"    solar(to - from, 0.0) {\n"
"        Cl = intensity * lightcolor;\n"
"    }\n"
"}\n";

const char* const POINTLIGHT =
"light pointlight(float intensity = 1; color lightcolor = 1;\n"
"                 point from = point \"shader\" (0, 0, 0);) {\n"
"    illuminate(from) {\n"
"        Cl = intensity * lightcolor / (L . L);\n"
"    }\n"
"}\n";

const char* const SPOTLIGHT =
"light spotlight(float intensity = 1; color lightcolor = 1;\n"
"                point from = point \"shader\" (0, 0, 0);\n"
"                point to = point \"shader\" (0, 0, 1);\n"
"                float coneangle = 0.1; float conedeltaangle = 0.02;\n"
"                float beamdistribution = 2;) {\n"
"    float atten;\n"
"    float cosangle;\n"
"    vector A = (to - from) / length(to - from);\n"
"    illuminate(from, A, coneangle) {\n"
"        cosangle = (L . A) / length(L);\n"
"        atten = pow(cosangle, beamdistribution) / (L . L);\n"
"        atten = atten * smoothstep(cos(coneangle), cos(coneangle - conedeltaangle), cosangle);\n"
"        Cl = atten * intensity * lightcolor;\n"
"    }\n"
"}\n";

const char* const BACKGROUND =
"imager background(color bgcolor = 0;) {\n"
"    Ci = Ci + (1 - alpha) * bgcolor;\n"
"    Oi = 1;\n"
"}\n";

};  // namespace

/**
 * The nine shaders compiled into the library all pass the checker.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_standard_shaders_test) {
    const char* const sources[] = {
        CONSTANT, MATTE, METAL, PLASTIC,
        AMBIENTLIGHT, DISTANTLIGHT, POINTLIGHT, SPOTLIGHT, BACKGROUND
    };
    for (const char* const source : sources) {
        BOOST_CHECK_EQUAL(compile(source), "");
    }
}

/**
 * A float promotes into anything made of floats by replication, so a shader can write
 * "color specularcolor = 1". The three point-like types convert to each other, and a colour
 * converts to none of them.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_coercion_test) {
    BOOST_CHECK_EQUAL(compile("surface s(color c = 1;) { Ci = c; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { point p = 0; Ci = Cs; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { vector V = N; Ci = Cs; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { normal M = I; Ci = Cs; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { matrix m = 1; Ci = Cs; }"), "");

    // a colour is not a position; a shader converts between them with ctransform
    BOOST_CHECK_EQUAL(compile("surface s() { point p = Cs; Ci = Cs; }"),
        "'p' is point and is given color at line 1, column 21");
    BOOST_CHECK_EQUAL(compile("surface s() { color c = P; Ci = Cs; }"),
        "'c' is color and is given point at line 1, column 21");
    // nor is a string
    BOOST_CHECK_EQUAL(compile("surface s() { float f = \"world\"; Ci = Cs; }"),
        "'f' is float and is given string at line 1, column 21");
}

/**
 * The two operators that differ from C. A dot product is a number and
 * a cross product is a direction, and neither takes a colour.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_dot_and_cross_test) {
    BOOST_CHECK_EQUAL(compile("surface s() { float f = N . I; Ci = Cs; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { vector V = N ^ I; Ci = Cs; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { float f = N ^ I; Ci = Cs; }"),
        "'f' is float and is given vector at line 1, column 21");
    BOOST_CHECK_EQUAL(compile("surface s() { float f = Cs . Os; Ci = Cs; }"),
        "'.' takes two positions or directions, not color and color at line 1, column 28");
}

/**
 * A parenthesised list is a literal for the type in front of it, and the count has to suit.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_tuple_test) {
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = color (1, 0, 0); }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { point p = point \"world\" (0, 1, 2); Ci = Cs; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = color (1, 0); }"),
        "color is 3 values, and 2 were given at line 1, column 26");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = (1, 0, 0); }"),
        "a parenthesised list of values needs a type in front of it at line 1, column 20");
}

/**
 * A space in front of a cast is a coordinate space for a position or a direction and a colour
 * space for a colour. A matrix relative to a space is not supported, and a float or a string
 * has no space to be in.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_cast_space_test) {
    BOOST_CHECK_EQUAL(compile("surface s() { vector v = vector \"world\" (0, 1, 0); Ci = Cs; }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = color \"rgb\" (1, 0, 0); }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = color \"hsv\" (1, 0, 0); }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { matrix m = matrix \"world\" 1; Ci = Cs; }"),
        "a matrix relative to a named space is not supported at line 1, column 26");
    BOOST_CHECK_EQUAL(compile("surface s() { float f = float \"world\" 1; Ci = Cs; }"),
        "a coordinate space means nothing to a float at line 1, column 25");
    BOOST_CHECK_EQUAL(compile("surface s() { string n = string \"rgb\" \"x\"; Ci = Cs; }"),
        "a coordinate space means nothing to a string at line 1, column 26");
}

/**
 * The componentwise maths returns the type its arguments promote to, as an arithmetic
 * operator does. A float beside a colour gives a colour, and a colour beside a point has no
 * arithmetic and is rejected.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_promoted_builtin_test) {
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = max(0, Cs); }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = abs(Cs); }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { point p = floor(P); Ci = pow(Cs, 2); }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { float f = max(0, Cs); Ci = Cs; }"),
        "'f' is float and is given color at line 1, column 21");
    BOOST_CHECK_EQUAL(compile("surface s() { float f = mix(0, Cs, 0.5); Ci = Cs; }"),
        "'f' is float and is given color at line 1, column 21");
    BOOST_CHECK(compile("surface s() { Ci = max(Cs, P); }").starts_with(
        "'max' cannot be called with those arguments"));
    // the trigonometry is declared for floats only
    BOOST_CHECK(compile("surface s() { Ci = sin(Cs); }").starts_with(
        "'sin' cannot be called with those arguments"));
}

/**
 * A shader may only write the globals its type owns. A light shader assigning Ci is told that
 * Ci belongs to a surface and an imager, rather than that the name is undeclared.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_global_writability_test) {
    BOOST_CHECK_EQUAL(compile("light l() { Ci = 1; }"),
        "'Ci' belongs to a surface and an imager shader, not to a light shader at line 1, column 13");
    BOOST_CHECK_EQUAL(compile("light l() { Cl = 1; }"), "");

    // a global its type does own may still be read only
    BOOST_CHECK_EQUAL(compile("surface s() { P = 0; Ci = Cs; }"),
        "'P' cannot be assigned in a surface shader at line 1, column 15");
    BOOST_CHECK_EQUAL(compile("surface s() { Cs = 1; Ci = Cs; }"),
        "'Cs' cannot be assigned in a surface shader at line 1, column 15");
    BOOST_CHECK_EQUAL(compile("imager i() { P = point (0, 0, 0); }"),
        "'P' cannot be assigned in an imager shader at line 1, column 14");
    // an imager writes alpha as well as reading it
    BOOST_CHECK_EQUAL(compile("imager i() { alpha = 1; }"), "");
}

/**
 * A surface shader's L and Cl are set by the light whose shader illuminance is running, and
 * mean nothing outside that body.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_lighting_globals_test) {
    BOOST_CHECK_EQUAL(compile("surface s() { illuminance(P) { Ci = Ci + Cl; } }"), "");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = Cl; }"),
        "'Cl' is set by a light and means nothing outside an 'illuminance' body "
        "at line 1, column 20");
    // each construct belongs to either the surface side or the light side
    BOOST_CHECK_EQUAL(compile("light l() { illuminance(P) { Cl = 1; } }"),
        "'illuminance' is only valid in a surface shader at line 1, column 13");
    BOOST_CHECK_EQUAL(compile("surface s() { illuminate(P) { Ci = 1; } }"),
        "'illuminate' is only valid in a light shader at line 1, column 15");
}

BOOST_AUTO_TEST_CASE(slcompiler_undeclared_identifier_test) {
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = missing; }"),
        "'missing' is not declared at line 1, column 20");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = wobble(Cs); }"),
        "'wobble' is not a function at line 1, column 20");
    BOOST_CHECK_EQUAL(compile("surface s() { Ci = normalize(\"world\"); }"),
        "'normalize' cannot be called with those arguments at line 1, column 20");
}

/**
 * A global is varying because it differs per shading point; a parameter is uniform unless it
 * was declared otherwise, because a scene binds one value for the whole primitive.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_storage_defaults_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s(float Ka = 1; varying float Kd = 1;) {\n"
        "    float a = Ka;\n"
        "    float b = s * 2;\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    BOOST_CHECK(storageOf(symbols, "P") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "E") == Storage::UNIFORM);
    BOOST_CHECK(storageOf(symbols, "Ka") == Storage::UNIFORM);
    BOOST_CHECK(storageOf(symbols, "Kd") == Storage::VARYING);
    // a local takes what reached it
    BOOST_CHECK(storageOf(symbols, "a") == Storage::UNIFORM);
    BOOST_CHECK(storageOf(symbols, "b") == Storage::VARYING);
}

/**
 * Different points take different arms, so anything assigned inside control flow with a
 * varying condition is varying whatever was assigned to it. Getting this wrong is silent.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_varying_condition_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s(float Ka = 1;) {\n"
        "    float inside = 0;\n"
        "    float outside = 0;\n"
        "    if (s > 0.5) { inside = Ka; }\n"
        "    if (Ka > 0.5) { outside = Ka; }\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    // the value assigned is uniform in both, and the condition is what differs
    BOOST_CHECK(storageOf(symbols, "inside") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "outside") == Storage::UNIFORM);
}

/**
 * A while loop's varying condition reaches its body the same way, and so does a for loop's.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_varying_loop_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s(float Ka = 1;) {\n"
        "    float counted = 0;\n"
        "    float stepped = 0;\n"
        "    float i = 0;\n"
        "    float j = 0;\n"
        "    while (i < t) { counted = Ka; i += 1; }\n"
        "    for (j = 0; j < Ka; j += 1) { stepped = Ka; }\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    BOOST_CHECK(storageOf(symbols, "counted") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "stepped") == Storage::UNIFORM);
    // the counter of the varying while is varying too: it is written under a varying
    // condition, so different points leave that loop having counted different numbers
    BOOST_CHECK(storageOf(symbols, "i") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "j") == Storage::UNIFORM);
}

/**
 * The inference runs to a fixed point rather than once. A loop carries a varying value back
 * to a name that was read before it was written, so one pass in source order would leave
 * `early` uniform with a varying value in it.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_fixed_point_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s(float Ka = 1;) {\n"
        "    float carried = 0;\n"
        "    float early = 0;\n"
        "    float i = 0;\n"
        "    for (i = 0; i < 4; i += 1) {\n"
        "        early = carried;\n"
        "        carried = s;\n"
        "    }\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    BOOST_CHECK(storageOf(symbols, "carried") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "early") == Storage::VARYING);
}

/**
 * An illuminance body is a loop over the lights, so a break inside it leaves that loop and
 * not the loop around it. The outer loop's body stays uniform, so a uniform declared there
 * still compiles.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_break_in_illuminance_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    float i = 0;\n"
        "    color C = 0;\n"
        "    for (i = 0; i < 2; i += 1) {\n"
        "        uniform float k = 1;\n"
        "        illuminance(P) {\n"
        "            C += Cl;\n"
        "            break;\n"
        "        }\n"
        "    }\n"
        "    Ci = C;\n"
        "}\n", &symbols), "");

    BOOST_CHECK(storageOf(symbols, "i") == Storage::UNIFORM);
    BOOST_CHECK(storageOf(symbols, "C") == Storage::VARYING);
}

/**
 * A string is uniform, and so is the coordinate space name inside a cast: there is no
 * per-point transform to look up, and allowing one would make every transform a runtime
 * string lookup.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_string_is_uniform_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    string space = \"world\";\n"
        "    point origin = point \"world\" (0, 0, 0);\n"
        "    point here = point \"world\" P;\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    BOOST_CHECK(storageOf(symbols, "space") == Storage::UNIFORM);
    BOOST_CHECK(storageOf(symbols, "origin") == Storage::UNIFORM);
    // the space is a name, and P makes this one vary
    BOOST_CHECK(storageOf(symbols, "here") == Storage::VARYING);
}

/**
 * A built-in that reads the shading point is varying however uniform its arguments are:
 * ambient() takes none at all and returns a different value at every point on a grid.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_varying_builtin_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s(float Ka = 1;) {\n"
        "    color lit = Ka * ambient();\n"
        "    float scaled = pow(Ka, 2);\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    BOOST_CHECK(storageOf(symbols, "lit") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "scaled") == Storage::UNIFORM);
}

/**
 * A built-in that returns results through its arguments writes them as an assignment would,
 * so they take the storage of what it read. Anything but a variable there cannot be
 * written, and is an error rather than silently dropped.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_written_arguments_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s(float eta = 0.5;) {\n"
        "    float kr; float kt; float flatr; float flatt;\n"
        "    fresnel(I, N, eta, kr, kt);\n"
        "    fresnel(vector (0, 0, -1), normal (0, 0, 1), eta, flatr, flatt);\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");
    BOOST_CHECK(storageOf(symbols, "kr") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "kt") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "flatr") == Storage::UNIFORM);

    BOOST_CHECK_EQUAL(compile("surface s() { float kt; fresnel(I, N, 0.5, 1, kt); Ci = Cs; }"),
        "argument 4 of 'fresnel' is written, so it has to be a variable at line 1, column 44");
    BOOST_CHECK_EQUAL(compile("surface s() { float kr; fresnel(I, N, 0.5, kr, s); Ci = Cs; }"),
        "argument 5 of 'fresnel' is written, and 's' cannot be assigned at line 1, column 48");
}

/**
 * A setter changes its first argument in place. That argument is written as an assignment
 * would write it: it has to be a variable the shader may assign, and it takes the storage of
 * the value written into it.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_updated_argument_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    point p = point (0, 0, 0);\n"
        "    point q = point (0, 0, 0);\n"
        "    color c = 0;\n"
        "    setxcomp(p, s);\n"
        "    setycomp(q, 1);\n"
        "    setcomp(c, 1, t);\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");
    BOOST_CHECK(storageOf(symbols, "p") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "q") == Storage::UNIFORM);
    BOOST_CHECK(storageOf(symbols, "c") == Storage::VARYING);

    BOOST_CHECK_EQUAL(compile("surface s() { setxcomp(P + I, 1); Ci = Cs; }"),
        "argument 1 of 'setxcomp' is written, so it has to be a variable at line 1, column 26");
    BOOST_CHECK_EQUAL(compile("surface s() { setxcomp(P, 1); Ci = Cs; }"),
        "argument 1 of 'setxcomp' is written, and 'P' cannot be assigned at line 1, column 24");
}

/**
 * Storage inference runs until nothing changes. Each assignment here reads a name that is
 * only written further down, so each round of inference reaches one more link of the chain,
 * and a chain of a hundred needs a hundred rounds.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_inference_reaches_the_end_of_a_long_chain_test) {
    const int links = 100;
    std::string source = "surface s() {\n";
    for (int i = 0; i <= links; i++) {
        source += "    float a" + std::to_string(i) + " = 0;\n";
    }
    source += "    float i = 0;\n    while (i < 2) {\n";
    for (int i = 0; i < links; i++) {
        source += "        a" + std::to_string(i) + " = a" + std::to_string(i + 1) + ";\n";
    }
    source += "        a" + std::to_string(links) + " = s;\n        i += 1;\n    }\n    Ci = Cs;\n}\n";

    std::vector<Symbol> symbols;
    BOOST_REQUIRE_EQUAL(compile(source, &symbols), "");
    BOOST_CHECK(storageOf(symbols, "a" + std::to_string(links)) == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "a0") == Storage::VARYING);
}

/**
 * A shader that declares a value uniform and then puts a varying one in it is an error.
 * Silently keeping it uniform would give a whole grid one point's value.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_uniform_given_a_varying_test) {
    BOOST_CHECK_EQUAL(compile("surface s() { uniform float f = s; Ci = Cs; }"),
        "'f' is uniform and is given a varying value at line 1, column 33");
    // the same through a varying condition rather than a varying value
    BOOST_CHECK_EQUAL(compile("surface s() { uniform float f = 0; if (t > 0) { f = 1; } Ci = Cs; }"),
        "'f' is uniform and is given a varying value at line 1, column 53");
}

/**
 * A function's formal takes the storage of every argument any call site passes it, so a
 * function called once with a varying value is varying wherever it is called.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_function_storage_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s(float Ka = 1;) {\n"
        "    float twice(float x) { return x + x; }\n"
        "    float fromParameter = twice(Ka);\n"
        "    float fromGlobal = twice(s);\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    BOOST_CHECK(storageOf(symbols, "x") == Storage::VARYING);
    BOOST_CHECK(storageOf(symbols, "fromGlobal") == Storage::VARYING);
    // the result carries it back to the uniform call site, because there is one body
    BOOST_CHECK(storageOf(symbols, "fromParameter") == Storage::VARYING);
}

/**
 * A function that reaches itself is rejected, because the machine has no call stack. The
 * compiler rejects it from the call graph, not the parser.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_recursion_test) {
    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    float loop(float x) { return loop(x); }\n"
        "    Ci = Cs;\n"
        "}\n"),
        "'loop' calls itself, and a shader run has no call stack at line 2, column 5");

    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    float a(float x) { return b(x); }\n"
        "    float b(float x) { return a(x); }\n"
        "    Ci = Cs;\n"
        "}\n"),
        "'a' calls itself, and a shader run has no call stack at line 2, column 5");
}

/**
 * A function is checked like anything else: its arity, its argument types and what it returns.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_function_checks_test) {
    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    float sqr(float x) { return x * x; }\n"
        "    Ci = Cs * sqr(0.5);\n"
        "}\n"), "");

    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    float sqr(float x) { return x * x; }\n"
        "    Ci = Cs * sqr(0.5, 1);\n"
        "}\n"),
        "'sqr' takes 1 arguments, and 2 were given at line 3, column 15");

    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    float bad(float x) { return \"world\"; }\n"
        "    Ci = Cs;\n"
        "}\n"),
        "'bad' returns float, not string at line 2, column 26");
}

/**
 * A parameter has to be a name of its own: one that collides with a global would give a
 * shader two things called Cs.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_parameter_names_test) {
    BOOST_CHECK_EQUAL(compile("surface s(color Cs = 1;) { Ci = Cs; }"),
        "'Cs' is already a shader global at line 1, column 11");
    BOOST_CHECK_EQUAL(compile("surface s(float a = 1; float a = 2;) { Ci = Cs; }"),
        "'a' is already a parameter at line 1, column 24");
    // its default has to be of its declared type
    BOOST_CHECK_EQUAL(compile("surface s(float a = \"world\";) { Ci = Cs; }"),
        "the default for 'a' is string, which is not float at line 1, column 11");
}

/**
 * Displacement and volume shaders parse and are refused here by name, so a scene naming one
 * is reported as unsupported rather than malformed.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_unsupported_shader_test) {
    BOOST_CHECK_EQUAL(compile("displacement bumpy(float Km = 1;) { }"),
        "a displacement shader is not supported at line 1, column 1");
    BOOST_CHECK_EQUAL(compile("volume fog(float density = 1;) { }"),
        "a volume shader is not supported at line 1, column 1");
}

/**
 * Every symbol the shader holds, in the order the machine allocates them: globals, then the
 * parameters a scene binds, then the locals.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_symbol_order_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile("surface s(float Ka = 1;) { float a = Ka; Ci = Cs; }", &symbols), "");

    BOOST_REQUIRE(symbols.size() > 3u);
    BOOST_CHECK(symbols.front().role == Symbol::Role::GLOBAL);
    BOOST_CHECK_EQUAL(symbols.front().name, "P");
    BOOST_CHECK(symbols[symbols.size() - 2].role == Symbol::Role::PARAMETER);
    BOOST_CHECK_EQUAL(symbols[symbols.size() - 2].name, "Ka");
    BOOST_CHECK(symbols.back().role == Symbol::Role::LOCAL);
    BOOST_CHECK_EQUAL(symbols.back().name, "a");
}

/**
 * A local in a nested block is its own symbol, because the machine allocates a register
 * against the symbol rather than against the name.
 **/
BOOST_AUTO_TEST_CASE(slcompiler_shadowing_test) {
    std::vector<Symbol> symbols;
    BOOST_CHECK_EQUAL(compile(
        "surface s() {\n"
        "    float a = 0;\n"
        "    if (1) { float a = s; }\n"
        "    Ci = Cs;\n"
        "}\n", &symbols), "");

    int found = 0;
    Storage outer = Storage::UNSPECIFIED;
    for (const Symbol & symbol : symbols) {
        if (symbol.name == "a") {
            if (found == 0) {
                outer = symbol.storage;
            }
            found++;
        }
    }
    BOOST_CHECK_EQUAL(found, 2);
    // the inner one takes the varying value and the outer one is untouched by it
    BOOST_CHECK(outer == Storage::UNIFORM);
}
