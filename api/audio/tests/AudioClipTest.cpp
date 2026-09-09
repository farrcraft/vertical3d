/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/AudioClip.h>

#include <fstream>
#include <string>

#include <boost/test/unit_test.hpp>

#include "Wav.h"

/**
 * A clip that read its file holds the wav it decoded.
 **/
BOOST_AUTO_TEST_CASE(audio_clip_load_test) {
    v3dtest::writeWav("clip.wav");

    v3d::audio::AudioClip clip;
    BOOST_TEST(clip.load("clip.wav"));
    BOOST_TEST(clip.audio() != nullptr);
}

/**
 * A missing file is a false return and no audio, so a caller that trusts the return is not
 * handed a clip that plays nothing.
 **/
BOOST_AUTO_TEST_CASE(audio_clip_missing_file_test) {
    v3d::audio::AudioClip clip;

    BOOST_TEST(!clip.load("nowhere.wav"));
    BOOST_TEST(clip.audio() == nullptr);
}

/**
 * A file that is not a wav is the same rejection as a file that is not there.
 **/
BOOST_AUTO_TEST_CASE(audio_clip_unreadable_file_test) {
    {
        std::ofstream out("notawav.wav", std::ios::binary | std::ios::trunc);
        out << "this is not a wav";
    }

    v3d::audio::AudioClip clip;
    BOOST_TEST(!clip.load("notawav.wav"));
    BOOST_TEST(clip.audio() == nullptr);
}

/**
 * A clip holds nothing until it is asked to load.
 **/
BOOST_AUTO_TEST_CASE(audio_clip_empty_test) {
    v3d::audio::AudioClip clip;

    BOOST_TEST(clip.audio() == nullptr);
    BOOST_CHECK_NO_THROW(clip.destroy());
}
