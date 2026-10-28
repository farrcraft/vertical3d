/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Attributes.h"

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::moya {

void Attributes::illuminate(const std::string & handle, bool on) {
    const std::vector<std::string>::iterator found = std::find(lit.begin(), lit.end(), handle);
    if (on && found == lit.end()) {
        lit.push_back(handle);
    } else if (!on && found != lit.end()) {
        lit.erase(found);
    }
}

bool Attributes::shines(const std::string & handle) const {
    return std::find(lit.begin(), lit.end(), handle) != lit.end();
}

};  // namespace v3d::moya
