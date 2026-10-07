/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <glm/common.hpp>

namespace v3d::type::animation {

/**
 * A value keyed in time and lerped between its keys: a colour or a size over a particle's life,
 * or a tint over a day.
 *
 * Without a period a track holds its first key before it and its last key after it. With one,
 * time wraps over the period and the last key leads back into the first, so a day's track
 * needs no repeated key at midnight.
 *
 * T is anything glm::mix takes: a float, or a glm vector.
 **/
template <typename T>
class Track final {
 public:
    /**
     **/
    struct Key final {
        float time;
        T value;
    };

    /**
     * A track that is the same value at every time.
     **/
    explicit Track(const T& constant) :
        keys_{Key{0.0f, constant}},
        period_(0.0f) {
    }

    /**
     * @param keys in rising time. With a period, every key lies in [0, period)
     * @param period the length time wraps over, or zero for a track that does not wrap
     * @throw std::invalid_argument for no keys, keys out of order, or a key outside the period
     **/
    explicit Track(const std::vector<Key>& keys, float period = 0.0f) :
        keys_(keys),
        period_(period) {
        if (keys_.empty()) {
            throw std::invalid_argument("a track needs at least one key");
        }
        if (period_ < 0.0f) {
            throw std::invalid_argument("a track's period cannot be negative");
        }
        for (std::size_t key = 0; key < keys_.size(); key++) {
            if (key > 0 && !(keys_[key - 1].time < keys_[key].time)) {
                throw std::invalid_argument("a track's keys must rise in time");
            }
            if (period_ > 0.0f && (keys_[key].time < 0.0f || keys_[key].time >= period_)) {
                throw std::invalid_argument("a wrapping track's keys must lie within its period");
            }
        }
    }

    /**
     * @return the value at a time
     **/
    T sample(float time) const {
        const Key& first = keys_.front();
        const Key& last = keys_.back();
        // a time that is not finite fails every comparison below, so it would fall through to
        // a search that finds no key; it reads as the first key instead
        if (!std::isfinite(time)) {
            return first.value;
        }
        if (period_ > 0.0f) {
            time = std::fmod(time, period_);
            if (time < 0.0f) {
                time += period_;
            }
            // across the wrap, from the last key to the first one a period later
            if (time < first.time || time >= last.time) {
                const float span = first.time + period_ - last.time;
                if (!(span > 0.0f)) {
                    return last.value;
                }
                const float since = time >= last.time ? time - last.time : time + period_ - last.time;
                return glm::mix(last.value, first.value, since / span);
            }
        } else {
            if (time <= first.time) {
                return first.value;
            }
            if (time >= last.time) {
                return last.value;
            }
        }
        const auto after = std::upper_bound(keys_.begin(), keys_.end(), time,
            [](float at, const Key& key) { return at < key.time; });
        const Key& to = *after;
        const Key& from = *(after - 1);
        return glm::mix(from.value, to.value, (time - from.time) / (to.time - from.time));
    }

    /**
     **/
    const std::vector<Key>& keys() const noexcept {
        return keys_;
    }

    /**
     * @return the length time wraps over, or zero
     **/
    float period() const noexcept {
        return period_;
    }

 private:
    std::vector<Key> keys_;
    float period_;
};

};  // namespace v3d::type::animation
