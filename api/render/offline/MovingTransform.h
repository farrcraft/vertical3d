/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline {

/**
 * A current transformation that may move across the shutter: its value when a motion block
 * opened and when it closed, and the times it named for each.
 *
 * Only a transform moves. Inside a block, each transform request applies to its own copy of
 * the transformation as it was when the block opened. The first request sets the open end and
 * the last sets the close; a block naming more than two times keeps its first and last.
 * Outside a block, a request applies to both ends, so what follows a block moves with it.
 *
 * The motion between the ends is translation and scale interpolated linearly and rotation by a
 * quaternion, which is exact for a rigid motion and a uniform scale. A shear, or a non-uniform
 * scale under a rotation, is interpolated approximately. An end that flattens the primitive has
 * no rotation to take apart, and the two ends are then blended as matrices.
 **/
class MovingTransform final {
 public:
    /** Still, at the identity. **/
    MovingTransform();
    /** Still, at one transformation. **/
    explicit MovingTransform(const glm::mat4x4 & still);

    bool moving() const;
    const glm::mat4x4 & open() const;
    const glm::mat4x4 & close() const;
    /**
     * The end a moving primitive is stored at, and moved from to any other time by
     * at(time) * inverse(reference()). The open end, unless it has no inverse, as when it
     * scales an axis to nothing; then the close end.
     **/
    const glm::mat4x4 & reference() const;
    /**
     * Whether the reference end has an inverse. False only when both ends flatten the
     * primitive. A moving primitive is then dropped, because nothing stored at the reference
     * end can be moved to another time. A still one needs no inverse and is drawn as it is.
     **/
    bool placeable() const;
    /** The times the motion block named for its two ends. **/
    const glm::vec2 & times() const;

    /**
     * The transformation at a time, held at its ends outside the block's times. A still
     * transformation returns its matrix exactly.
     **/
    glm::mat4x4 at(float time) const;

    /**
     * Both ends followed by a matrix - a request outside a block, or a camera applied after.
     **/
    MovingTransform after(const glm::mat4x4 & matrix) const;
    /** A matrix followed by both ends, which is how the camera transformation applies. **/
    MovingTransform before(const glm::mat4x4 & matrix) const;

    /**
     * RiMotionBegin. The requests up to end() each apply to the next end in turn.
     **/
    void begin(const std::vector<float> & times);
    bool inBlock() const;
    /** RiMotionEnd. **/
    void end();

    /** A request that concatenates a matrix, such as RiTranslate or RiConcatTransform. **/
    void concat(const glm::mat4x4 & matrix);
    /** A request that replaces the transformation, such as RiTransform or RiIdentity. **/
    void replace(const glm::mat4x4 & matrix);

 private:
    /**
     * Decomposes both ends, which at() then only blends. Called whenever the ends change
     * while the transformation moves.
     **/
    void settle();

    glm::mat4x4 open_;
    glm::mat4x4 close_;
    glm::vec2 times_ { 0.0f, 0.0f };
    bool moving_ { false };
    /** Whether an end has no inverse, so at() blends the two matrices rather than their parts. **/
    bool linear_ { false };

    glm::vec3 openTranslation_ { 0.0f };
    glm::vec3 closeTranslation_ { 0.0f };
    glm::vec3 openScale_ { 1.0f };
    glm::vec3 closeScale_ { 1.0f };
    glm::quat openRotation_ { 1.0f, 0.0f, 0.0f, 0.0f };
    glm::quat closeRotation_ { 1.0f, 0.0f, 0.0f, 0.0f };

    /** A block's state: the transformation when it opened, and each request's result so far. **/
    bool inBlock_ { false };
    std::vector<float> blockTimes_;
    glm::mat4x4 baseOpen_;
    glm::mat4x4 baseClose_;
    std::vector<glm::mat4x4> ends_;
};

};  // namespace v3d::render::offline
