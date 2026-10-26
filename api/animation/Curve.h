/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <vector>

namespace v3d::animation {

/**
 * One number keyed on frames, which an animator shapes key by key.
 *
 * Each key says how the curve leaves it for the next key. It is held until the next key, drawn
 * as a straight line, or drawn as a cubic. The cubic's slopes at its two ends are the first
 * key's out slope and the second key's in slope. A slope is in value per frame. Before the first
 * key the curve holds the first key's value, and after the last key it holds the last key's.
 *
 * An automatic key has its slopes worked out again whenever the keys change. Its slope is that
 * of the line through the keys either side of it. It is flat at a peak, a trough, the first key
 * and the last key, so the curve does not overshoot them.
 **/
class Curve final {
 public:
    enum class Interpolation : std::uint8_t {
        Step,     /**< held until the next key **/
        Linear,   /**< a straight line to the next key **/
        Bezier    /**< a cubic to the next key, by the two keys' slopes **/
    };

    struct Key final {
        float frame = 0.0F;
        float value = 0.0F;
        /** How the curve leaves this key for the next one. **/
        Interpolation interpolation = Interpolation::Bezier;
        /** The slope the curve arrives with, from the key before. **/
        float in = 0.0F;
        /** The slope the curve leaves with, toward the key after. **/
        float out = 0.0F;
        /** Whether the slopes are worked out from the neighbours. **/
        bool automatic = true;

        bool operator==(const Key& other) const = default;
    };

    /**
     * Two frames closer than this are the same frame, so a key set there replaces the key on it.
     **/
    static constexpr float SAME_FRAME = 1e-4F;

    /**
     * A curve with no keys, which is 0 everywhere.
     **/
    Curve();

    /**
     * @param keys in rising frame, each finite. The slopes of the automatic ones are worked out
     * @throw std::invalid_argument for keys out of order or not finite
     **/
    explicit Curve(const std::vector<Key>& keys);

    /**
     * @return the value at a frame, or 0 when there are no keys. A frame that is not finite reads
     *         as the first key
     **/
    float sample(float frame) const;

    /**
     * Key a value at a frame. A key already on that frame takes the value and keeps its
     * interpolation and slopes. Otherwise a new automatic Bezier key is added in its place.
     **/
    void set(float frame, float value);

    /**
     * @return whether there was a key on that frame to remove
     **/
    bool remove(float frame);

    bool empty() const noexcept;

    const std::vector<Key>& keys() const noexcept;

    bool operator==(const Curve& other) const = default;

 private:
    /**
     * Work out the slopes of every automatic key from its neighbours.
     **/
    void smooth();

    std::vector<Key> keys_;
};

};  // namespace v3d::animation
