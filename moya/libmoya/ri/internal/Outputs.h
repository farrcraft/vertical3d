/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/rib/Parameters.h>
#include <api/rib/requests/Display.h>
#include <api/rib/requests/DisplayChannel.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace v3d::moya::ri::internal {

/**
 * The scene's outputs and channels, from RI's Display, RenderMan's DisplayChannel and its
 * PxrCryptomatte sample filter. Each output is named after its file, and each channel after the
 * name its request gives it. It reads the requests alone and reports what it refuses, so anything
 * that sends RI requests can fill a scene's outputs through it.
 **/
class Outputs final {
 public:
    /** @param scene made with scene::kinds(), which outlives this **/
    explicit Outputs(scene::Scene * scene);

    /**
     * Add or replace an output. A display whose name starts with `+` adds one, and any other
     * replaces every output before it. A file ending in `.png` is written as PNG, and any other
     * as EXR, unless the type is `"framebuffer"`, or `"deepexr"` for a deep file. The mode names
     * the channels the output holds beside the beauty, apart by commas. `rgb`, `rgba`, `Ci` and
     * `a` name the beauty. Any other name is a declared channel's, or a channel of that name whose
     * source is the name.
     **/
    void display(const v3d::rib::requests::Display & request);

    /**
     * Declare a channel of the declaration's last word, of three components for a colour, point,
     * vector or normal as `rib::Declaration` reads the type, and one for any other type or none.
     * Its `"string source"` is what it collects, after the source's type: `lpe:` and a light path
     * expression, or a value at the first hit. A channel of no source collects the value its own
     * name names. Its `"string filter"` is how its samples combine: `min`, `max`, `sum`, or `zmin`
     * for the closest. Its `"int denoise"` of 1 writes it denoised beside it. A channel declared
     * again is replaced in every output that names it.
     *
     * @return false, making nothing, when the declaration names no channel
     **/
    bool declare(const v3d::rib::requests::DisplayChannel & request, const v3d::rib::ParameterList & parameters);

    /**
     * Add the output of a PxrCryptomatte's `"string filename"`, in floats, of one layer. Its
     * `"string layer"` names what the IDs name: `Name` or `object` an object, `Material` or
     * `material` a material, and `Path` or `asset` an asset. The layer's channel is named
     * CryptoObject, CryptoMaterial or CryptoAsset, and a second filter of the layer names the
     * same channel.
     *
     * @return false, making nothing, when the layer is none of those
     **/
    bool cryptomatte(const v3d::rib::ParameterList & parameters);

 private:
    /** An output, and the channels it names beside the beauty. **/
    struct Output {
        scene::Handle handle{};
        std::vector<scene::Handle> channels{};
    };

    /** The output named after a file, made when there is none. **/
    Output & output(const std::string & file);

    /** Set the channels an output names. **/
    void setChannels(Output * output, const std::vector<scene::Handle> & channels);

    /**
     * A new channel of a name that collects a source: `lpe:` and an expression, or a value.
     * RenderMan's z, P and Nn are read as the depth, the position and the normal. Components
     * above zero set its own.
     **/
    scene::Handle channelOf(const std::string & name, const std::string & source, std::int32_t components);

    /** Name a channel in place of another in each output that names the other. **/
    void replace(const scene::Handle & old, const scene::Handle & channel);

    scene::Scene * scene_;
    std::map<std::string, Output> outputs_;
    std::map<std::string, scene::Handle> channels_;
};

};  // namespace v3d::moya::ri::internal
