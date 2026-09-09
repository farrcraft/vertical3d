/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Text.h"

#include <sstream>
#include <string>
#include <vector>

namespace v3d::ui::paint {

/**
 **/
std::vector<std::string> wrap(std::string_view line, float width, const Measure& measure) {
    std::vector<std::string> rows;
    std::istringstream words = std::istringstream(std::string(line));
    std::string word;
    std::string row;
    while (words >> word) {
        if (row.empty()) {
            row = word;
            continue;
        }
        if (width <= 0.0f) {
            row.append(" ").append(word);
            continue;
        }
        std::string wider(row);
        wider.append(" ").append(word);
        if (measure(wider) <= width) {
            row = wider;
        } else {
            rows.push_back(row);
            row = word;
        }
    }
    if (!row.empty()) {
        rows.push_back(row);
    }
    return rows;
}

};  // namespace v3d::ui::paint
