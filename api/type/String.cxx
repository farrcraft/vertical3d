/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "String.h"

#include <cassert>
#include <string>
#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/algorithm/string.hpp>

namespace v3d::type {

glm::vec2 string_to_vec2(const std::string& val) {
    glm::vec2 vec(0.0f);
    if (!val.empty()) {
        std::string var_x;
        std::string var_y;
        size_t pos;
        pos = val.find(',');
        var_x = val.substr(0, pos);
        boost::trim(var_x);
        var_y = val.substr(pos + 1, val.length());
        boost::trim(var_y);

        // a component that does not parse keeps the zero it was built with, which is what
        // try_lexical_convert reports rather than throws
        float parsed = 0.0f;
        if (boost::conversion::try_lexical_convert(var_x, parsed)) {
            vec[0] = parsed;
        }
        if (boost::conversion::try_lexical_convert(var_y, parsed)) {
            vec[1] = parsed;
        }
    }
    return vec;
}

};  // namespace v3d::type
