/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/Film.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/Sampling.h>

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>

namespace {

typedef v3d::render::offline::Filter Filter;

float weight(Filter kind, float x, float y, float width) {
    return v3d::render::offline::filter(kind, glm::vec2(x, y), glm::vec2(width));
}

/** How many samples a box filter takes along each axis, and how wide it is. **/
struct Box {
    unsigned int samples = 1;
    float width = 1.0F;
};

v3d::render::offline::Sampling box(const Box & filter) {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(filter.samples);
    sampling.filter = Filter::Box;
    sampling.width = glm::vec2(filter.width);
    return sampling;
}

/**
 * Each filter at its centre, at half its width and past it, against RI's formulas worked by
 * hand. A box and a triangle two pixels wide reach a pixel either side. The gaussian is
 * exp(-2) at its edge.
 **/
BOOST_AUTO_TEST_CASE(film_filter_weights_test) {
    BOOST_CHECK_EQUAL(weight(Filter::Box, 0.0F, 0.0F, 2.0F), 1.0F);
    BOOST_CHECK_EQUAL(weight(Filter::Box, 1.0F, -1.0F, 2.0F), 1.0F);
    BOOST_CHECK_EQUAL(weight(Filter::Box, 1.01F, 0.0F, 2.0F), 0.0F);

    BOOST_CHECK_EQUAL(weight(Filter::Triangle, 0.0F, 0.0F, 2.0F), 1.0F);
    BOOST_CHECK_CLOSE(weight(Filter::Triangle, 0.5F, 0.0F, 2.0F), 0.5F, 1.0e-4F);
    BOOST_CHECK_CLOSE(weight(Filter::Triangle, 0.5F, 0.5F, 2.0F), 0.25F, 1.0e-4F);
    BOOST_CHECK_EQUAL(weight(Filter::Triangle, 1.0F, 0.0F, 2.0F), 0.0F);

    BOOST_CHECK_EQUAL(weight(Filter::Gaussian, 0.0F, 0.0F, 2.0F), 1.0F);
    BOOST_CHECK_CLOSE(weight(Filter::Gaussian, 1.0F, 0.0F, 2.0F), std::exp(-2.0F), 1.0e-4F);
    BOOST_CHECK_EQUAL(weight(Filter::Gaussian, 1.5F, 0.0F, 2.0F), 0.0F);

    // catmull-rom is radial, reaches zero two pixels out, is 2 at the centre and has a
    // negative lobe
    BOOST_CHECK_EQUAL(weight(Filter::CatmullRom, 0.0F, 0.0F, 4.0F), 2.0F);
    BOOST_CHECK_CLOSE(weight(Filter::CatmullRom, 0.5F, 0.0F, 4.0F), 1.125F, 1.0e-4F);
    BOOST_CHECK_SMALL(weight(Filter::CatmullRom, 1.0F, 0.0F, 4.0F), 1.0e-6F);
    BOOST_CHECK_CLOSE(weight(Filter::CatmullRom, 1.5F, 0.0F, 4.0F), -0.125F, 1.0e-3F);
    BOOST_CHECK_EQUAL(weight(Filter::CatmullRom, 2.5F, 0.0F, 4.0F), 0.0F);

    BOOST_CHECK_EQUAL(weight(Filter::Sinc, 0.0F, 0.0F, 4.0F), 1.0F);
    BOOST_CHECK_CLOSE(weight(Filter::Sinc, 0.5F, 0.0F, 4.0F), 2.0F / 3.14159265F, 1.0e-3F);
    BOOST_CHECK_SMALL(weight(Filter::Sinc, 1.0F, 0.0F, 4.0F), 1.0e-6F);
    BOOST_CHECK_EQUAL(weight(Filter::Sinc, 2.5F, 0.0F, 4.0F), 0.0F);
}

/**
 * Samples of one colour resolve to that colour wherever the filter reaches, and to full
 * coverage, including at the image's edges where part of the filter falls outside it.
 **/
BOOST_AUTO_TEST_CASE(film_uniform_test) {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(3, 3);
    const v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(4, 3, sampling);

    const glm::vec3 colour(0.25F, 0.5F, 0.75F);
    for (unsigned int row = 0; row < 3; row++) {
        for (unsigned int column = 0; column < 4; column++) {
            for (const v3d::render::offline::Sampler::Sample & s : sampler.pixel(column, row)) {
                v3d::render::offline::Film::Sample sample;
                sample.raster = s.raster;
                sample.colour = colour;
                sample.opacity = glm::vec3(1.0F);
                sample.hit = true;
                film.add(sample);
            }
        }
    }

    for (unsigned int row = 0; row < 3; row++) {
        for (unsigned int column = 0; column < 4; column++) {
            const glm::vec3 resolved = film.colour(column, row);
            BOOST_CHECK_CLOSE(resolved.r, colour.r, 1.0e-4F);
            BOOST_CHECK_CLOSE(resolved.g, colour.g, 1.0e-4F);
            BOOST_CHECK_CLOSE(resolved.b, colour.b, 1.0e-4F);
            BOOST_CHECK_CLOSE(film.coverage(column, row), 1.0F, 1.0e-4F);
            BOOST_CHECK_CLOSE(film.opacity(column, row).g, 1.0F, 1.0e-4F);
        }
    }
}

/**
 * Under a box, a pixel where half the samples hit has a coverage of a half. The coverage
 * plane holds that fraction. A miss adds black to the colour, so the colour is premultiplied.
 **/
BOOST_AUTO_TEST_CASE(film_half_coverage_test) {
    const v3d::render::offline::Sampling sampling = box({.samples = 2, .width = 1.0F});
    const v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(1, 1, sampling);

    const std::vector<v3d::render::offline::Sampler::Sample> samples = sampler.pixel(0, 0);
    BOOST_REQUIRE_EQUAL(samples.size(), 4U);
    for (const v3d::render::offline::Sampler::Sample & s : samples) {
        v3d::render::offline::Film::Sample sample;
        sample.raster = s.raster;
        sample.hit = s.raster.x < 0.5F;
        sample.colour = glm::vec3(sample.hit ? 1.0F : 0.0F);
        film.add(sample);
    }

    BOOST_CHECK_EQUAL(film.coverage(0, 0), 0.5F);
    BOOST_CHECK_EQUAL(film.colour(0, 0).r, 0.5F);
}

/**
 * One sample a pixel under a box one pixel wide resolves to the sample exactly, and the
 * sample is at the pixel's centre.
 **/
BOOST_AUTO_TEST_CASE(film_one_sample_is_exact_test) {
    const v3d::render::offline::Sampling sampling = box({.samples = 1, .width = 1.0F});
    const v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(3, 2, sampling);

    for (unsigned int row = 0; row < 2; row++) {
        for (unsigned int column = 0; column < 3; column++) {
            const std::vector<v3d::render::offline::Sampler::Sample> samples = sampler.pixel(column, row);
            BOOST_REQUIRE_EQUAL(samples.size(), 1U);
            BOOST_CHECK_EQUAL(samples[0].raster.x, static_cast<float>(column) + 0.5F);
            BOOST_CHECK_EQUAL(samples[0].raster.y, static_cast<float>(row) + 0.5F);

            v3d::render::offline::Film::Sample sample;
            sample.raster = samples[0].raster;
            // the middle pixel of the top row is a miss
            sample.hit = column != 1 || row != 0;
            if (sample.hit) {
                sample.colour = glm::vec3(0.1F * static_cast<float>(column), 0.3F, 0.7F / static_cast<float>(row + 1));
            }
            sample.depth = 2.0F + static_cast<float>(column);
            film.add(sample);
        }
    }

    v3d::render::offline::FrameBuffer frame(3, 2, 5);
    frame.clear({.plane = 3, .value = 99.0F});
    film.resolve(&frame, 0, 4, 3);

    BOOST_CHECK_EQUAL(frame.value(0, 2, 1), 0.1F * 2.0F);
    BOOST_CHECK_EQUAL(frame.value(1, 2, 1), 0.3F);
    BOOST_CHECK_EQUAL(frame.value(2, 2, 1), 0.7F / 2.0F);
    BOOST_CHECK_EQUAL(frame.value(3, 2, 1), 4.0F);
    BOOST_CHECK_EQUAL(frame.value(4, 2, 1), 1.0F);
    // the miss is black, uncovered, and keeps the depth its plane started with
    BOOST_CHECK_EQUAL(frame.value(0, 1, 0), 0.0F);
    BOOST_CHECK_EQUAL(frame.value(4, 1, 0), 0.0F);
    BOOST_CHECK_EQUAL(frame.value(3, 1, 0), 99.0F);
}

/**
 * A sample whose raster position is not finite, or lies far off the film on either side,
 * reaches no pixel: it adds no colour, no coverage and no depth.
 **/
BOOST_AUTO_TEST_CASE(film_sample_off_the_film_reaches_no_pixel_test) {
    const v3d::render::offline::Sampling sampling = box({.samples = 1, .width = 2.0F});
    v3d::render::offline::Film film(3, 2, sampling);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const std::vector<glm::vec2> positions = {
        glm::vec2(nan, 0.5F), glm::vec2(0.5F, nan), glm::vec2(infinity, 0.5F), glm::vec2(0.5F, -infinity),
        glm::vec2(1.0e12F, 0.5F), glm::vec2(-1.0e12F, 0.5F), glm::vec2(0.5F, 1.0e12F), glm::vec2(0.5F, -1.0e12F),
    };
    for (const glm::vec2& position : positions) {
        v3d::render::offline::Film::Sample sample;
        sample.raster = position;
        sample.colour = glm::vec3(1.0F);
        sample.hit = true;
        sample.depth = 1.0F;
        film.add(sample);
    }
    for (unsigned int row = 0; row < 2; row++) {
        for (unsigned int column = 0; column < 3; column++) {
            float depth = 0.0F;
            BOOST_CHECK(!film.depth(column, row, &depth));
            BOOST_CHECK_EQUAL(film.coverage(column, row), 0.0F);
            BOOST_CHECK_EQUAL(film.colour(column, row).r, 0.0F);
        }
    }
}

/**
 * On a film whose filter width is NaN, a sample is filtered into no pixel. A hit still records
 * its depth in the pixel it lies in.
 **/
BOOST_AUTO_TEST_CASE(film_nan_filter_width_reaches_no_pixel_test) {
    const v3d::render::offline::Sampling sampling = box({.samples = 1, .width = std::numeric_limits<float>::quiet_NaN()});
    v3d::render::offline::Film film(3, 2, sampling);
    v3d::render::offline::Film::Sample sample;
    sample.raster = glm::vec2(1.5F, 0.5F);
    sample.colour = glm::vec3(1.0F);
    sample.hit = true;
    sample.depth = 2.0F;
    film.add(sample);
    for (unsigned int row = 0; row < 2; row++) {
        for (unsigned int column = 0; column < 3; column++) {
            BOOST_CHECK_EQUAL(film.coverage(column, row), 0.0F);
            BOOST_CHECK_EQUAL(film.colour(column, row).r, 0.0F);
        }
    }
    float depth = 0.0F;
    BOOST_CHECK(film.depth(1, 0, &depth));
    BOOST_CHECK_EQUAL(depth, 2.0F);
}

/**
 * A pixel's samples are the same whatever order pixels are requested in and on a second
 * sampler, fall inside the pixel, and differ from one pass to the next.
 **/
BOOST_AUTO_TEST_CASE(film_sampler_is_deterministic_test) {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(4, 4);
    sampling.shutter = glm::vec2(0.0F, 0.5F);

    const v3d::render::offline::Sampler first(sampling);
    const std::vector<v3d::render::offline::Sampler::Sample> before = first.pixel(3, 5);
    first.pixel(0, 0);
    first.pixel(7, 2);
    const std::vector<v3d::render::offline::Sampler::Sample> after = first.pixel(3, 5);
    const v3d::render::offline::Sampler second(sampling);
    const std::vector<v3d::render::offline::Sampler::Sample> again = second.pixel(3, 5);

    BOOST_REQUIRE_EQUAL(before.size(), 16U);
    for (std::size_t i = 0; i < before.size(); i++) {
        BOOST_CHECK_EQUAL(before[i].raster.x, after[i].raster.x);
        BOOST_CHECK_EQUAL(before[i].raster.y, after[i].raster.y);
        BOOST_CHECK_EQUAL(before[i].time, again[i].time);
        BOOST_CHECK_EQUAL(before[i].lens.x, again[i].lens.x);

        BOOST_CHECK(before[i].raster.x >= 3.0F && before[i].raster.x < 4.0F);
        BOOST_CHECK(before[i].raster.y >= 5.0F && before[i].raster.y < 6.0F);
        BOOST_CHECK(before[i].time >= 0.0F && before[i].time <= 0.5F);
        BOOST_CHECK(glm::dot(before[i].lens, before[i].lens) <= 1.0001F);
    }

    const std::vector<v3d::render::offline::Sampler::Sample> next = first.pixel(3, 5, 1);
    BOOST_CHECK(next[0].raster.x != before[0].raster.x || next[0].raster.y != before[0].raster.y);
}

/**
 * Samples are stratified: one falls in each cell of the grid PixelSamples requests.
 **/
BOOST_AUTO_TEST_CASE(film_sampler_is_stratified_test) {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(3, 2);
    const v3d::render::offline::Sampler sampler(sampling);

    std::vector<unsigned int> cells(6, 0);
    for (const v3d::render::offline::Sampler::Sample & s : sampler.pixel(10, 20)) {
        const unsigned int column = static_cast<unsigned int>((s.raster.x - 10.0F) * 3.0F);
        const unsigned int row = static_cast<unsigned int>((s.raster.y - 20.0F) * 2.0F);
        BOOST_REQUIRE(column < 3 && row < 2);
        cells[(row * 3) + column]++;
    }
    for (const unsigned int count : cells) {
        BOOST_CHECK_EQUAL(count, 1U);
    }
}

};  // namespace
