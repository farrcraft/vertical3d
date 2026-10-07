/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

namespace v3d::render::offline::sl::runtime {

/**
 * What one instruction does.
 *
 * A flat list over register indices rather than a tree the machine traverses, for two
 * reasons: the mask handling below needs jump targets, and a tree traversal over a batch
 * allocates at every node.
 **/
enum class Opcode {
    /** target = left, converting as the two registers' types call for. **/
    MOVE,
    /**
     * One component of target takes the number in left, `right` saying which. A
     * parenthesised list is a literal for the type in front of it, and this instruction
     * writes each of its values into its own component.
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
    /** The dot product, which SL writes '.' - a number out of two directions. **/
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
     * the registers holding its arguments. Its implementation belongs to the library, which
     * is passed to the machine rather than held by it.
     **/
    CALL,
    /**
     * A cast with a named coordinate space: target = left, stated in the space in the string
     * register `right`, moved into current space. That is the inverse of the matrix the
     * renderer returns for the space. The target register's type picks the transform: a point
     * translates, a vector does not, a normal goes by the inverse transpose.
     **/
    TRANSFORM,
    /** Jump to `target`. **/
    JUMP,
    /**
     * Jump to `target` when the **uniform** register `left` is zero. A condition with the
     * same value at every point is a jump rather than a mask, so the common case costs no
     * mask operations.
     **/
    JUMP_IF_ZERO,
    /**
     * Push the lanes of `left` that are not zero, and jump to `target` when none are - an
     * arm no point takes costs the push and nothing else.
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
     * Put the loop's lanes back without leaving the body: what a for loop puts in front of
     * its step, since a continue goes to the step rather than past it.
     **/
    LOOP_RESTORE,
    /** The bottom of a loop body: put the loop's lanes back and jump to `target`. **/
    LOOP_END,
    POP_LOOP,
    /**
     * Take the live lanes out of the loop for good, or for the rest of this iteration.
     * Neither jumps: a lane leaves by losing its bit, because the lanes beside it have not
     * finished.
     **/
    BREAK,
    CONTINUE,
    /**
     * Open an inlined function body.
     *
     * A call is inlined because a run has no call stack. The body is bracketed rather than
     * merely pasted in, because a `return` inside it means "these lanes are done with this
     * function", not "done with this shader".
     **/
    ENTER,
    /** Close an inlined body: the lanes that returned from it are live again. **/
    LEAVE,
    /**
     * Open an illuminance loop. `left` and `right` are the surface shader's L and Cl, and
     * `arguments` are the construct's own - the point being shaded, and a cone axis and
     * half angle when it named one.
     *
     * The surface shader's side of the message passing between surface and light shaders:
     * the body runs once per light, over the same batch, with L and Cl set by that light's
     * own program.
     **/
    ILLUMINANCE,
    /**
     * Set L and Cl from the next light the body has not run for, and narrow the batch to
     * the points that light reaches inside the cone. Jump to `target` when the lights run
     * out. A light that reaches no point of this batch is skipped, so a scene with a
     * hundred lights does not run the body a hundred times over an empty mask.
     **/
    ILLUMINANCE_NEXT,
    POP_ILLUMINANCE,
    /**
     * The other end of the message passing, in a light shader with a position. L is from
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
     * Take the live lanes out of the innermost inlined body, or out of everything when the
     * shader body is what is running. Like BREAK it does not jump, because the lanes beside
     * it have not finished.
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
    /** A call's arguments, and nothing else's. **/
    std::vector<int> arguments;
    /** Where in the source this came from, for a runtime report. **/
    unsigned int line = 0;
    unsigned int column = 0;
};

};  // namespace v3d::render::offline::sl::runtime
