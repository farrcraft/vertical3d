/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Text.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::editor::control {

/**
 **/
std::string lower(std::string_view text) {
    std::string made(text);
    std::ranges::transform(made, made.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });
    return made;
}

/**
 **/
std::vector<std::string> words(std::string_view query) {
    std::vector<std::string> made;
    std::string word;
    for (const char letter : query) {
        if (std::isspace(static_cast<unsigned char>(letter)) != 0) {
            if (!word.empty()) {
                made.push_back(lower(word));
            }
            word.clear();
        } else {
            word += letter;
        }
    }
    if (!word.empty()) {
        made.push_back(lower(word));
    }
    return made;
}

};  // namespace v3d::editor::control
