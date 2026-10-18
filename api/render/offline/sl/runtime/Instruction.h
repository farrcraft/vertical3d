/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <vector>

namespace v3d::render::offline::sl::runtime {

/**
 * What one instruction does.
 *
 * A program is a flat list of instructions over register indices, not a syntax tree. The
 * mask handling below needs jump targets, and walking a tree over a batch would allocate at
 * every node.
 **/
enum class Opcode : std::uint8_t {
    /** target = left, converting as the two registers' types call for. **/
    MOVE,
    /**
     * Write the number in `left` into the component of `target` that `right` names. A
     * parenthesised list is a literal for the type in front of it, and one of these
     * instructions writes each value of the list into its own component.
     **/
    MOVE_COMPONENT,
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    /** target = -left. **/
    NEGATE,
    /** target = left == 0. **/
    NOT,
    /** The dot product of two directions, which SL writes '.'. **/
    DOT,
    /** The cross product, which SL writes '^'. **/
    CROSS,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    EQUAL,
    NOT_EQUAL,
    AND,
    OR,
    /**
     * A standard library function: `left` is its index in builtins() and `arguments` are
     * the registers holding its arguments. Machine::builtin() runs it.
     **/
    CALL,
    /**
     * A cast with a named coordinate space. `left` is stated in the space named by the string
     * register `right`, and `target` receives it in current space. The transform is the
     * inverse of the matrix the renderer returns for the space. The target register's type
     * picks how it applies: a point translates, a vector does not, and a normal is
     * transformed by the inverse transpose.
     **/
    TRANSFORM,
    /** Jump to `target`. **/
    JUMP,
    /**
     * Jump to `target` when the **uniform** register `left` is zero. A condition with the
     * same value at every point compiles to this jump, not to a mask, so the common case
     * costs no mask operations.
     **/
    JUMP_IF_ZERO,
    /**
     * Push the live lanes where `left` is not zero. Jump to `target` when there are none, so
     * an arm that no point takes is skipped.
     **/
    MASK,
    /** The same for the lanes that are zero: the else arm. **/
    MASK_NOT,
    POP_MASK,
    /**
     * Begin a loop. `target` is the address of its POP_LOOP. A test that runs out of lanes
     * jumps there.
     **/
    LOOP,
    /**
     * Narrow the loop to the lanes where `left` is not zero, and leave it when none are.
     **/
    LOOP_TEST,
    /**
     * Restore the loop's lanes without leaving the body. A for loop emits this before its
     * step, because a continue goes to the step and not past it.
     **/
    LOOP_RESTORE,
    /** The bottom of a loop body: restore the loop's lanes and jump to `target`. **/
    LOOP_END,
    POP_LOOP,
    /**
     * Clear the live lanes from the loop: BREAK for good, CONTINUE for the rest of this
     * iteration. Neither jumps, because the other lanes have not finished.
     **/
    BREAK,
    CONTINUE,
    /**
     * Open an inlined function body.
     *
     * A call is inlined because a run has no call stack. ENTER and LEAVE bracket the body,
     * because a `return` inside it ends the function for its lanes, not the shader.
     **/
    ENTER,
    /** Close an inlined body: the lanes that returned from it are live again. **/
    LEAVE,
    /**
     * Open an illuminance loop. `left` and `right` are the surface shader's L and Cl, and
     * `arguments` are the construct's own: the point being shaded, then a cone axis and half
     * angle when it names a cone.
     *
     * This is the surface shader's side of the exchange with light shaders. The body runs
     * once per light, over the same batch, with L and Cl set by that light's own program.
     **/
    ILLUMINANCE,
    /**
     * Set L and Cl from the next light the body has not run for, and narrow the batch to
     * the points that light reaches inside the cone. Jump to `target` when the lights run
     * out. A light that reaches no point of this batch is skipped, so the body never runs
     * over an empty mask.
     **/
    ILLUMINANCE_NEXT,
    POP_ILLUMINANCE,
    /**
     * The light shader's side of the exchange, for a light with a position. L is from
     * the point being lit toward `arguments[0]`, and the batch narrows to the points inside
     * the cone `arguments[1]` and `arguments[2]` name. `left` and `right` are the light
     * shader's L and Ps. Jump to `target` when the light reaches no point at all.
     **/
    ILLUMINATE,
    /**
     * The same for a light at infinity, where every point is lit from one direction: L is
     * against `arguments[0]`, which is the direction the light travels in.
     **/
    SOLAR,
    /**
     * Clear the live lanes from the innermost inlined body, or from everything when no
     * inlined body is open. Like BREAK it does not jump, because the other lanes have not
     * finished.
     **/
    RETURN
};

class Instruction final {
 public:
    Opcode opcode = Opcode::MOVE;
    /** The register written, or the address jumped to. **/
    int target = -1;
    int left = -1;
    int right = -1;
    /** The argument registers of a CALL, ILLUMINANCE, ILLUMINATE or SOLAR. **/
    std::vector<int> arguments;
    /** Where in the source this came from. No runtime report reads it. **/
    unsigned int line = 0;
    unsigned int column = 0;
};

};  // namespace v3d::render::offline::sl::runtime
