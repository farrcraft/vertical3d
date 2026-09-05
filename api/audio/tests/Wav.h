/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace v3dtest {

    /**
     * Write the smallest wav the mixer will accept - a 44 byte canonical header over a handful
     * of 16 bit mono samples. Generated rather than committed so that every byte a case
     * depends on is visible here.
     **/
    inline void writeWav(const std::string& path, uint32_t samples = 16) {
        const uint32_t rate = 44100;
        const uint16_t channels = 1;
        const uint16_t bits = 16;
        const uint32_t data = samples * channels * (bits / 8);
        const uint32_t byteRate = rate * channels * (bits / 8);
        const uint16_t blockAlign = static_cast<uint16_t>(channels * (bits / 8));

        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        auto u32 = [&out](uint32_t value) {
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        };
        auto u16 = [&out](uint16_t value) {
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        };

        out.write("RIFF", 4);
        u32(36 + data);
        out.write("WAVE", 4);
        out.write("fmt ", 4);
        u32(16);
        u16(1);  // PCM
        u16(channels);
        u32(rate);
        u32(byteRate);
        u16(blockAlign);
        u16(bits);
        out.write("data", 4);
        u32(data);

        const std::vector<char> silence(data, 0);
        out.write(silence.data(), static_cast<std::streamsize>(silence.size()));
    }

};  // namespace v3dtest
