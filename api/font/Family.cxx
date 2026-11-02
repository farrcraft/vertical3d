/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Family.h"

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

#include "Face.h"
#include "FaceStyle.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font {

namespace {

/**
 * The weights CSS treats as the middle of the range, between which the order of preference
 * changes.
 **/
constexpr unsigned int MIDDLE_LOW = 400;
constexpr unsigned int MIDDLE_HIGH = 500;

/**
 * @return the face whose weight is nearest the one asked for among those the test accepts, or
 *         null when it accepts none. A face's weight is the one in its range nearest that asked
 *         for
 **/
boost::shared_ptr<Face> nearest(const std::vector<boost::shared_ptr<Face>>& faces, unsigned int weight,
    const std::function<bool(unsigned int)>& accepts) {
    boost::shared_ptr<Face> found;
    unsigned int best = 0;
    for (const boost::shared_ptr<Face>& face : faces) {
        const unsigned int each = std::clamp(weight, face->weights().first, face->weights().second);
        if (!accepts(each)) {
            continue;
        }
        const unsigned int distance = each > weight ? each - weight : weight - each;
        if (!found || distance < best) {
            found = face;
            best = distance;
        }
    }
    return found;
}

};  // namespace

Family::Family(const std::string& name) :
    name_(name) {
}

void Family::add(const boost::shared_ptr<Face>& face) {
    if (face) {
        faces_.push_back(face);
    }
}

const std::string& Family::name() const noexcept {
    return name_;
}

const std::vector<boost::shared_ptr<Face>>& Family::faces() const noexcept {
    return faces_;
}

bool Family::empty() const noexcept {
    return faces_.empty();
}

boost::shared_ptr<Face> Family::face(const FaceStyle& style) const {
    const unsigned int weight = style.weight;
    std::vector<boost::shared_ptr<Face>> styled;
    for (const boost::shared_ptr<Face>& each : faces_) {
        if (each->italic() == style.italic) {
            styled.push_back(each);
        }
    }
    const std::vector<boost::shared_ptr<Face>>& candidates = styled.empty() ? faces_ : styled;

    if (weight >= MIDDLE_LOW && weight <= MIDDLE_HIGH) {
        boost::shared_ptr<Face> found = nearest(candidates, weight, [weight](unsigned int each) { return each >= weight && each <= MIDDLE_HIGH; });
        if (!found) {
            found = nearest(candidates, weight, [weight](unsigned int each) { return each < weight; });
        }
        return found ? found : nearest(candidates, weight, [](unsigned int each) { return each > MIDDLE_HIGH; });
    }
    if (weight < MIDDLE_LOW) {
        const boost::shared_ptr<Face> found = nearest(candidates, weight, [weight](unsigned int each) { return each <= weight; });
        return found ? found : nearest(candidates, weight, [weight](unsigned int each) { return each > weight; });
    }
    const boost::shared_ptr<Face> found = nearest(candidates, weight, [weight](unsigned int each) { return each >= weight; });
    return found ? found : nearest(candidates, weight, [weight](unsigned int each) { return each < weight; });
}

};  // namespace v3d::font
