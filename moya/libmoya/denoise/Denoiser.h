/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Channel.h>

#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::denoise {

namespace internal {
struct Device;
};  // namespace internal

/**
 * OIDN's CPU device, kept for the pictures it denoises. Each is filtered by OIDN's RT filter,
 * guided by the albedo and the normal the snapshot's channels hold, of the input scale scaleOf()
 * gives it. A picture with no albedo is filtered alone. A filter that fails is reported, and
 * gives the noisy picture.
 *
 * One thread uses a denoiser at a time.
 **/
class Denoiser final {
 public:
    explicit Denoiser(const boost::shared_ptr<log::Logger> & logger);

    /**
     * Denoises a snapshot into the denoised copies its film holds: the beauty's when denoising it,
     * and each denoised channel's. The guides are filtered first, and every copy is then filtered
     * with cleanAux set.
     *
     * @param channels the channels the snapshot's film was made from, the guides among them
     **/
    void into(film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool beauty);

    /**
     * A snapshot's beauty denoised, three floats a pixel, row by row from the top left. A preview
     * is filtered at OIDN's fast quality from the guides as they are. Any other is filtered as
     * into() filters the beauty, and holds the same bytes.
     **/
    std::vector<float> beauty(const film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool preview);

 private:
    boost::shared_ptr<internal::Device> device_;
    boost::shared_ptr<log::Logger> logger_;
};

};  // namespace v3d::moya::denoise
