/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Migration.h"

#include <cstddef>
#include <utility>

#include <boost/system/error_code.hpp>

namespace v3d::asset {

namespace {

const char* const VERSION = "version";

};  // namespace

Reading readForward(boost::json::object* document, int current, std::span<const Migration> chain) {
    if (document == nullptr || current < 1) {
        return Reading::Refused;
    }
    const boost::json::value* stamped = document->if_contains(VERSION);
    if (stamped == nullptr) {
        return Reading::Refused;
    }
    // to_number accepts 2.0 as 2 and refuses 2.5, a string and a bool, because a person may
    // have typed the version by hand
    boost::system::error_code error;
    const int version = stamped->to_number<int>(error);
    if (error || version < 1) {
        return Reading::Refused;
    }
    if (version > current) {
        return Reading::Newer;
    }
    if (version == current) {
        return Reading::Current;
    }
    // the steps from this version to the current one have to all be there before any runs
    if (static_cast<std::size_t>(current - 1) > chain.size()) {
        return Reading::Refused;
    }

    boost::json::object walked = *document;
    for (int from = version; from < current; ++from) {
        const Migration& step = chain[static_cast<std::size_t>(from - 1)];
        if (!step || !step(walked)) {
            return Reading::Refused;
        }
        walked[VERSION] = from + 1;
    }
    *document = std::move(walked);
    return Reading::Migrated;
}

};  // namespace v3d::asset
