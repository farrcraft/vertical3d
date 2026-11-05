/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// Passes each input to the RIB reader as a whole stream, and records every request it reads.
// The reader stops at what it cannot parse, so the target checks only that no input makes the
// lexer or the reader misbehave.

#include <api/log/Logger.h>
#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/rib/Recorder.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

#include <boost/make_shared.hpp>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    static const boost::shared_ptr<v3d::log::Logger> logger = [] {
        auto created = boost::make_shared<v3d::log::Logger>();
        // a rejected input is the common case, and logging each one only slows the run
        created->get()->set_level(spdlog::level::off);
        return created;
    }();
    std::istringstream stream(std::string(reinterpret_cast<const char*>(data), size));
    v3d::render::offline::rib::Reader reader(logger);
    v3d::render::offline::rib::Recorder recorder;
    reader.read(stream, &recorder);
    return 0;
}
