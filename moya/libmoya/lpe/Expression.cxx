/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/lpe/Expression.h>

#include <moya/libmoya/lpe/Failure.h>
#include <moya/libmoya/lpe/internal/Reader.h>
#include <moya/libmoya/lpe/Pattern.h>

#include <string>
#include <variant>
#include <vector>

namespace v3d::moya::lpe {

std::variant<Pattern, Failure> parse(const std::string & text, const std::vector<std::string> & labels) {
    try {
        return internal::Reader(text, labels).whole();
    } catch (const internal::Reader::Stopped & stopped) {
        return Failure { .place = stopped.place, .reason = stopped.reason };
    }
}

};  // namespace v3d::moya::lpe
