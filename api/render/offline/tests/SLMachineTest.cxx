/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Emitter.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/runtime/Machine.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::render::offline::sl::runtime::Opcode Opcode;
typedef v3d::render::offline::sl::runtime::Program Program;
typedef v3d::render::offline::sl::runtime::Register Register;
typedef v3d::render::offline::sl::runtime::Instruction Instruction;
typedef v3d::render::offline::sl::Type Type;
typedef v3d::render::offline::sl::Storage Storage;

/**
 * Read a shader, check it and emit it. The three passes are separate classes and this is the
 * only place a case wants all of them at once.
 **/
bool build(const std::string & source, Program* program, std::string* error) {
    std::istringstream stream(source);
    v3d::render::offline::sl::Parser parser(stream);
    std::vector<v3d::render::offline::sl::ShaderPtr> shaders = parser.parse();
    if (shaders.size() != 1) {
        *error = parser.error();
        return false;
    }
    v3d::render::offline::sl::Compiler compiler(shaders[0]);
    if (!compiler.compile()) {
        *error = compiler.error();
        return false;
    }
    v3d::render::offline::sl::Emitter emitter(shaders[0], compiler.symbols());
    if (!emitter.emit(program)) {
        *error = emitter.error();
        return false;
    }
    return true;
}

/**
 * A register the program computes into.
 **/
int add(Program* program, Type type, Storage storage, const std::string & name = std::string()) {
    Register reg;
    reg.type = type;
    reg.storage = storage;
    reg.name = name;
    program->registers.push_back(reg);
    return static_cast<int>(program->registers.size()) - 1;
}

int constant(Program* program, float value) {
    Register reg;
    reg.type = Type::FLOAT;
    reg.storage = Storage::UNIFORM;
    reg.constant = true;
    reg.value.push_back(value);
    program->registers.push_back(reg);
    return static_cast<int>(program->registers.size()) - 1;
}

int put(Program* program, Opcode opcode, int target, int left, int right = -1) {
    Instruction instruction;
    instruction.opcode = opcode;
    instruction.target = target;
    instruction.left = left;
    instruction.right = right;
    program->instructions.push_back(instruction);
    return static_cast<int>(program->instructions.size()) - 1;
}

/**
 * How many instructions of that opcode a program holds - which is how a case says that a
 * uniform condition compiled to a jump rather than to a mask.
 **/
std::size_t count(const Program & program, Opcode opcode) {
    std::size_t found = 0;
    for (const Instruction & instruction : program.instructions) {
        if (instruction.opcode == opcode) {
            found++;
        }
    }
    return found;
}

};  // namespace

/**
 * The point of the batch: a hand-built program computes the same arithmetic over four points
 * and over one, and answers the same thing at the point they share. talyn's single hit is not
 * a special case of the model, it is a batch one wide.
 **/
BOOST_AUTO_TEST_CASE(slmachine_batch_of_one_test) {
    Program program;
    const int x = add(&program, Type::FLOAT, Storage::VARYING, "x");
    const int scale = constant(&program, 3.0f);
    const int offset = constant(&program, 1.0f);
    const int scaled = add(&program, Type::FLOAT, Storage::VARYING);
    const int result = add(&program, Type::FLOAT, Storage::VARYING, "result");
    put(&program, Opcode::MULTIPLY, scaled, x, scale);
    put(&program, Opcode::ADD, result, scaled, offset);
    program.symbols = program.registers.size();

    v3d::render::offline::sl::runtime::Machine wide;
    wide.prepare(program, 4);
    for (unsigned int point = 0; point < 4; point++) {
        wide.value(x).number(point, static_cast<float>(point));
    }
    BOOST_REQUIRE(wide.run(program));
    BOOST_CHECK_EQUAL(wide.value(result).number(0), 1.0f);
    BOOST_CHECK_EQUAL(wide.value(result).number(2), 7.0f);
    BOOST_CHECK_EQUAL(wide.value(result).number(3), 10.0f);

    v3d::render::offline::sl::runtime::Machine single;
    single.prepare(program, 1);
    single.value(x).number(0, 2.0f);
    BOOST_REQUIRE(single.run(program));
    // the same instructions, a mask one bit wide, and the answer the grid gave at that point
    BOOST_CHECK_EQUAL(single.value(result).number(0), 7.0f);
    BOOST_CHECK_EQUAL(single.batch(), 1u);
}

/**
 * A uniform value is stored once and read by every point, which is the memory the storage
 * class decides.
 **/
BOOST_AUTO_TEST_CASE(slmachine_uniform_is_one_element_test) {
    Program program;
    const int once = add(&program, Type::FLOAT, Storage::UNIFORM, "once");
    const int each = add(&program, Type::FLOAT, Storage::VARYING, "each");
    put(&program, Opcode::MOVE, each, once);
    program.symbols = program.registers.size();

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 8);
    BOOST_CHECK_EQUAL(machine.value(once).width(), 1u);
    BOOST_CHECK_EQUAL(machine.value(each).width(), 8u);

    machine.value(once).number(0, 5.0f);
    BOOST_REQUIRE(machine.run(program));
    BOOST_CHECK_EQUAL(machine.value(each).number(0), 5.0f);
    BOOST_CHECK_EQUAL(machine.value(each).number(7), 5.0f);
}

/**
 * A colour is three floats and a float promotes into it by replication, which is what
 * "Ci = Os * 0.5" leans on.
 **/
BOOST_AUTO_TEST_CASE(slmachine_promotion_test) {
    Program program;
    const int colour = add(&program, Type::COLOR, Storage::VARYING, "colour");
    const int half = constant(&program, 0.5f);
    const int dimmed = add(&program, Type::COLOR, Storage::VARYING, "dimmed");
    put(&program, Opcode::MULTIPLY, dimmed, colour, half);
    program.symbols = program.registers.size();

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 2);
    machine.value(colour).triple(0, glm::vec3(1.0f, 0.5f, 0.0f));
    machine.value(colour).triple(1, glm::vec3(0.0f, 1.0f, 1.0f));
    BOOST_REQUIRE(machine.run(program));

    BOOST_CHECK_EQUAL(machine.value(dimmed).triple(0).r, 0.5f);
    BOOST_CHECK_EQUAL(machine.value(dimmed).triple(0).g, 0.25f);
    BOOST_CHECK_EQUAL(machine.value(dimmed).triple(1).b, 0.5f);
}

/**
 * A varying condition runs both arms, each under the lanes that took it, so each lane comes
 * out with its own answer.
 **/
BOOST_AUTO_TEST_CASE(slmachine_varying_if_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    float answer = 0;\n"
        "    if (s > 0.5) { answer = 10; } else { answer = 20; }\n"
        "    Ci = color (answer, 0, 0);\n"
        "}\n", &program, &error), error);

    // the condition is varying, so it is a mask rather than a jump
    BOOST_CHECK_EQUAL(count(program, Opcode::MASK), 1u);
    BOOST_CHECK_EQUAL(count(program, Opcode::MASK_NOT), 1u);
    BOOST_CHECK_EQUAL(count(program, Opcode::JUMP_IF_ZERO), 0u);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 4);
    const int texture = program.symbol("s");
    const int answer = program.symbol("answer");
    BOOST_REQUIRE(texture >= 0 && answer >= 0);
    machine.value(texture).number(0, 0.0f);
    machine.value(texture).number(1, 1.0f);
    machine.value(texture).number(2, 0.0f);
    machine.value(texture).number(3, 1.0f);
    BOOST_REQUIRE(machine.run(program));

    BOOST_CHECK_EQUAL(machine.value(answer).number(0), 20.0f);
    BOOST_CHECK_EQUAL(machine.value(answer).number(1), 10.0f);
    BOOST_CHECK_EQUAL(machine.value(answer).number(2), 20.0f);
    BOOST_CHECK_EQUAL(machine.value(answer).number(3), 10.0f);
}

/**
 * A condition every point agrees about is a jump rather than a mask - the optimisation that
 * makes the common case free.
 **/
BOOST_AUTO_TEST_CASE(slmachine_uniform_if_is_a_jump_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s(float Ka = 1;) {\n"
        "    float answer = 0;\n"
        "    if (Ka > 0.5) { answer = 10; } else { answer = 20; }\n"
        "    Ci = color (answer, 0, 0);\n"
        "}\n", &program, &error), error);

    BOOST_CHECK_EQUAL(count(program, Opcode::JUMP_IF_ZERO), 1u);
    BOOST_CHECK_EQUAL(count(program, Opcode::MASK), 0u);
    BOOST_CHECK_EQUAL(count(program, Opcode::MASK_NOT), 0u);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 4);
    const int ka = program.symbol("Ka");
    const int answer = program.symbol("answer");
    machine.value(ka).number(0, 1.0f);
    BOOST_REQUIRE(machine.run(program));
    BOOST_CHECK_EQUAL(machine.value(answer).number(0), 10.0f);

    machine.value(ka).number(0, 0.0f);
    BOOST_REQUIRE(machine.run(program));
    BOOST_CHECK_EQUAL(machine.value(answer).number(0), 20.0f);
}

/**
 * An arm no lane took is skipped: the mask is pushed and the body jumped over, rather than
 * run over an empty batch.
 **/
BOOST_AUTO_TEST_CASE(slmachine_empty_mask_skips_an_arm_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    float taken = 0;\n"
        "    if (s > 0.5) { taken = 1; } else { taken = 2; }\n"
        "    Ci = color (taken, 0, 0);\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 4);
    const int texture = program.symbol("s");
    const int taken = program.symbol("taken");
    // no lane takes the first arm, so nothing it holds is written
    for (unsigned int point = 0; point < 4; point++) {
        machine.value(texture).number(point, 0.0f);
    }
    BOOST_REQUIRE(machine.run(program));
    for (unsigned int point = 0; point < 4; point++) {
        BOOST_CHECK_EQUAL(machine.value(taken).number(point), 2.0f);
    }
}

/**
 * A break in a varying while leaves the other lanes running: a lane goes by losing its bit,
 * because there is nowhere to jump to on its behalf while the lanes beside it are still in
 * the loop.
 **/
BOOST_AUTO_TEST_CASE(slmachine_break_leaves_the_others_running_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    float rounds = 0;\n"
        "    float i = 0;\n"
        "    while (i < 10) {\n"
        "        i += 1;\n"
        "        if (i > t) { break; }\n"
        "        rounds += 1;\n"
        "    }\n"
        "    Ci = color (rounds, 0, 0);\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 3);
    const int limit = program.symbol("t");
    const int rounds = program.symbol("rounds");
    machine.value(limit).number(0, 1.0f);
    machine.value(limit).number(1, 3.0f);
    machine.value(limit).number(2, 5.0f);
    BOOST_REQUIRE(machine.run(program));

    // each lane left at its own count, and the lanes after it kept going
    BOOST_CHECK_EQUAL(machine.value(rounds).number(0), 1.0f);
    BOOST_CHECK_EQUAL(machine.value(rounds).number(1), 3.0f);
    BOOST_CHECK_EQUAL(machine.value(rounds).number(2), 5.0f);
}

/**
 * A continue takes a lane out for the rest of the pass and gives it back at the next one,
 * which is the difference between it and a break.
 **/
BOOST_AUTO_TEST_CASE(slmachine_continue_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    float counted = 0;\n"
        "    float i = 0;\n"
        "    for (i = 0; i < 4; i += 1) {\n"
        "        if (i < t) { continue; }\n"
        "        counted += 1;\n"
        "    }\n"
        "    Ci = color (counted, 0, 0);\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 3);
    const int limit = program.symbol("t");
    const int counted = program.symbol("counted");
    machine.value(limit).number(0, 0.0f);
    machine.value(limit).number(1, 2.0f);
    machine.value(limit).number(2, 4.0f);
    BOOST_REQUIRE(machine.run(program));

    BOOST_CHECK_EQUAL(machine.value(counted).number(0), 4.0f);
    BOOST_CHECK_EQUAL(machine.value(counted).number(1), 2.0f);
    BOOST_CHECK_EQUAL(machine.value(counted).number(2), 0.0f);
}

/**
 * A loop iterates while any lane is live, so a batch whose lanes finish at different times
 * runs until the last of them is done.
 **/
BOOST_AUTO_TEST_CASE(slmachine_loop_runs_while_any_lane_lives_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    float total = 0;\n"
        "    float i = 0;\n"
        "    while (i < t) { total += i; i += 1; }\n"
        "    Ci = color (total, 0, 0);\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 3);
    const int limit = program.symbol("t");
    const int total = program.symbol("total");
    machine.value(limit).number(0, 0.0f);
    machine.value(limit).number(1, 3.0f);
    machine.value(limit).number(2, 5.0f);
    BOOST_REQUIRE(machine.run(program));

    BOOST_CHECK_EQUAL(machine.value(total).number(0), 0.0f);
    BOOST_CHECK_EQUAL(machine.value(total).number(1), 3.0f);
    BOOST_CHECK_EQUAL(machine.value(total).number(2), 10.0f);
}

/**
 * The dot product and the cross product, which the language writes '.' and '^'.
 **/
BOOST_AUTO_TEST_CASE(slmachine_products_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    float d = N . I;\n"
        "    vector c = N ^ I;\n"
        "    Ci = color (d, 0, 0);\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 1);
    machine.value(program.symbol("N")).triple(0, glm::vec3(1.0f, 0.0f, 0.0f));
    machine.value(program.symbol("I")).triple(0, glm::vec3(0.0f, 1.0f, 0.0f));
    BOOST_REQUIRE(machine.run(program));

    BOOST_CHECK_EQUAL(machine.value(program.symbol("d")).number(0), 0.0f);
    const glm::vec3 crossed = machine.value(program.symbol("c")).triple(0);
    BOOST_CHECK_EQUAL(crossed.z, 1.0f);
}

/**
 * A cast fills its components from the list in front of it, and a compound assignment is the
 * operator and then the assignment.
 **/
BOOST_AUTO_TEST_CASE(slmachine_cast_and_compound_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    Ci = color (1, 0.5, 0.25);\n"
        "    Ci *= 2;\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 1);
    BOOST_REQUIRE(machine.run(program));

    const glm::vec3 colour = machine.value(program.symbol("Ci")).triple(0);
    BOOST_CHECK_EQUAL(colour.r, 2.0f);
    BOOST_CHECK_EQUAL(colour.g, 1.0f);
    BOOST_CHECK_EQUAL(colour.b, 0.5f);
}

/**
 * The whole of `constant`, which is the shader phase 2's renderers were doing by hand.
 **/
BOOST_AUTO_TEST_CASE(slmachine_constant_shader_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface constant() {\n"
        "    Ci = Cs;\n"
        "    Oi = Os;\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 2);
    machine.value(program.symbol("Cs")).triple(0, glm::vec3(1.0f, 0.0f, 0.0f));
    machine.value(program.symbol("Cs")).triple(1, glm::vec3(0.0f, 0.0f, 1.0f));
    machine.value(program.symbol("Os")).triple(0, glm::vec3(1.0f));
    machine.value(program.symbol("Os")).triple(1, glm::vec3(1.0f));
    BOOST_REQUIRE(machine.run(program));

    BOOST_CHECK_EQUAL(machine.value(program.symbol("Ci")).triple(0).r, 1.0f);
    BOOST_CHECK_EQUAL(machine.value(program.symbol("Ci")).triple(1).b, 1.0f);
    BOOST_CHECK_EQUAL(machine.value(program.symbol("Oi")).triple(1).g, 1.0f);
}

/**
 * A named coordinate space is the renderer's answer rather than the machine's, and a machine
 * with no renderer attached says so once rather than at every point of every grid.
 **/
BOOST_AUTO_TEST_CASE(slmachine_transform_needs_a_renderer_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    point origin = point \"world\" (0, 0, 0);\n"
        "    Ci = Cs;\n"
        "}\n", &program, &error), error);

    BOOST_CHECK_EQUAL(count(program, Opcode::TRANSFORM), 1u);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 4);
    BOOST_REQUIRE(machine.run(program));
    BOOST_REQUIRE_EQUAL(machine.reports().size(), 1u);
    BOOST_CHECK_EQUAL(machine.reports()[0],
        "the coordinate space \"world\" is not one this renderer knows");
}

/**
 * A shader with a loop nothing ends fails rather than hanging the render, because a hung
 * render says nothing about why.
 **/
BOOST_AUTO_TEST_CASE(slmachine_endless_loop_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build(
        "surface s() {\n"
        "    float i = 0;\n"
        "    while (1) { i += 1; }\n"
        "    Ci = Cs;\n"
        "}\n", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 1);
    BOOST_CHECK(!machine.run(program));
    BOOST_CHECK_EQUAL(machine.error(), "the shader 's' ran without end");
}

/**
 * A run reuses the register file rather than allocating one: a renderer shading a thousand
 * grids over one program prepares once and runs a thousand times.
 **/
BOOST_AUTO_TEST_CASE(slmachine_prepare_once_test) {
    std::string error;
    Program program;
    BOOST_REQUIRE_MESSAGE(build("surface s() { Ci = Cs * 2; }", &program, &error), error);

    v3d::render::offline::sl::runtime::Machine machine;
    machine.prepare(program, 2);
    for (int round = 1; round <= 3; round++) {
        machine.value(program.symbol("Cs")).triple(0, glm::vec3(static_cast<float>(round)));
        BOOST_REQUIRE(machine.run(program));
        BOOST_CHECK_EQUAL(machine.value(program.symbol("Ci")).triple(0).r,
            static_cast<float>(round) * 2.0f);
    }
}
