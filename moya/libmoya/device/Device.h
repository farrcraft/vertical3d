/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>

#include <cstddef>
#include <string>

#include <boost/shared_ptr.hpp>

/**
 * The devices moya renders on, and the memory each holds. Kernel code runs on a device. Host code
 * allocates its memory, copies data in and out, and launches it.
 **/
namespace v3d::moya::device {

/**
 * A processor moya renders on: the CPU, or one kind of GPU. What differs between devices is here:
 * how memory is held and copied.
 *
 * Each device launches kernel functions its own way, so a launch is on the device's own class. The
 * CPU calls a function directly, and a GPU starts the entry point it compiled.
 **/
class Device {
 public:
    virtual ~Device() = default;

    /** A name for the device, for the log. **/
    virtual std::string name() const = 0;

    /** A buffer of the given size, its contents undefined until something writes them. **/
    virtual boost::shared_ptr<Buffer> allocate(std::size_t bytes) = 0;

    /**
     * Copies bytes from host memory into the start of a buffer this device allocated.
     *
     * @throws std::out_of_range when the buffer holds fewer bytes
     **/
    virtual void upload(Buffer & to, const void * from, std::size_t bytes) = 0;

    /**
     * Copies bytes from the start of a buffer this device allocated into host memory.
     *
     * @throws std::out_of_range when the buffer holds fewer bytes
     **/
    virtual void download(const Buffer & from, void * to, std::size_t bytes) = 0;
};

};  // namespace v3d::moya::device
