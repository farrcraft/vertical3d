/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// Passes each input to the TGA reader as the bytes of a file. The reader rejects what it
// cannot decode, so the target checks only that no input makes it misbehave.

#include <api/image/reader/Tga.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <cstdint>

#include <boost/make_shared.hpp>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    static const boost::shared_ptr<v3d::log::Logger> logger = [] {
        auto created = boost::make_shared<v3d::log::Logger>();
        // a rejected input is the common case, and logging each one only slows the run
        created->get()->set_level(spdlog::level::off);
        return created;
    }();
    v3d::image::reader::Tga reader(logger);
    reader.read(data, size);
    return 0;
}
