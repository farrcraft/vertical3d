/*
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "Style.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::style {

/**
 * The values a theme wrote over a dressing, and what each held before.
 *
 * A theme names only some values. The next theme starts from the values before any theme,
 * not from what the last one left. A value the new theme does not set goes back to its
 * default, and no theme at all puts every value back.
 *
 * It records the field, not its address, so a dressing that is copied keeps working.
 * restore() also replaces any value the app wrote over a themed one.
 *
 * @param D the dressing the values are in
 **/
template <typename D>
class Overlay final {
 public:
    /**
     * Read a colour as readColour() does, and remember what it replaced.
     **/
    void colour(const boost::shared_ptr<Style>& target, const std::string& name, D* dressing, glm::vec4 D::* field) {
        const glm::vec4 was = dressing->*field;
        readColour(target, name, &(dressing->*field));
        if (dressing->*field != was) {
            colours_.emplace_back(field, was);
        }
    }

    /**
     * Write a colour that does not come from a property, and remember what it replaced.
     **/
    void set(D* dressing, glm::vec4 D::* field, const glm::vec4& value) {
        const glm::vec4 was = dressing->*field;
        dressing->*field = value;
        if (value != was) {
            colours_.emplace_back(field, was);
        }
    }

    /**
     * Read a number as readMetric() does, and remember what it replaced.
     *
     * @param target the style read, or null to read nothing
     * @param name the number property's name
     * @param dressing the dressing written to
     * @param field the member of the dressing the number goes into
     * @param scale what the number is multiplied by, for a size the theme gives in logical pixels
     **/
    void metric(const boost::shared_ptr<Style>& target, const std::string& name, D* dressing, float D::* field,
        float scale = 1.0F) {
        if (!target || !target->property(name, "number")) {
            return;
        }
        const float was = dressing->*field;
        float read = was;
        readMetric(target, name, &read);
        dressing->*field = read * scale;
        metrics_.emplace_back(field, was);
    }

    /**
     * Read a number as metric() does, and turn it to an outline's width at a scale with round.
     **/
    template <typename Round>
    void width(const boost::shared_ptr<Style>& target, const std::string& name, D* dressing, float D::* field,
        float scale, Round round) {
        if (!target || !target->property(name, "number")) {
            return;
        }
        const float was = dressing->*field;
        float read = was;
        readMetric(target, name, &read);
        dressing->*field = round(read, scale);
        metrics_.emplace_back(field, was);
    }

    /**
     * Put back every value read since the last restore, and forget them.
     **/
    void restore(D* dressing) {
        for (auto it = colours_.rbegin(); it != colours_.rend(); ++it) {
            dressing->*(it->first) = it->second;
        }
        for (auto it = metrics_.rbegin(); it != metrics_.rend(); ++it) {
            dressing->*(it->first) = it->second;
        }
        colours_.clear();
        metrics_.clear();
    }

 private:
    std::vector<std::pair<glm::vec4 D::*, glm::vec4>> colours_;
    std::vector<std::pair<float D::*, float>> metrics_;
};

};  // namespace v3d::ui::style
