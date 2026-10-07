/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/Film.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/Sampling.h>

#include <cmath>
#include <limits>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>

namespace {

typedef v3d::render::offline::Filter Filter;

float weight(Filter kind, float x, float y, float width) {
    return v3d::render::offline::filter(kind, glm::vec2(x, y), glm::vec2(width));
}

v3d::render::offline::Sampling box(unsigned int samples, float width) {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(samples);
    sampling.filter = Filter::Box;
    sampling.width = glm::vec2(width);
    return sampling;
}

};  // namespace

/**
 * Each filter at its centre, at half its width and past it, against RI's formulas worked by
 * hand. A box and a triangle two pixels wide reach a pixel either side; the gaussian is
 * exp(-2) at its edge.
 **/
BOOST_AUTO_TEST_CASE(film_filter_weights_test) {
    BOOST_CHECK_EQUAL(weight(Filter::Box, 0.0f, 0.0f, 2.0f), 1.0f);
    BOOST_CHECK_EQUAL(weight(Filter::Box, 1.0f, -1.0f, 2.0f), 1.0f);
    BOOST_CHECK_EQUAL(weight(Filter::Box, 1.01f, 0.0f, 2.0f), 0.0f);

    BOOST_CHECK_EQUAL(weight(Filter::Triangle, 0.0f, 0.0f, 2.0f), 1.0f);
    BOOST_CHECK_CLOSE(weight(Filter::Triangle, 0.5f, 0.0f, 2.0f), 0.5f, 1.0e-4f);
    BOOST_CHECK_CLOSE(weight(Filter::Triangle, 0.5f, 0.5f, 2.0f), 0.25f, 1.0e-4f);
    BOOST_CHECK_EQUAL(weight(Filter::Triangle, 1.0f, 0.0f, 2.0f), 0.0f);

    BOOST_CHECK_EQUAL(weight(Filter::Gaussian, 0.0f, 0.0f, 2.0f), 1.0f);
    BOOST_CHECK_CLOSE(weight(Filter::Gaussian, 1.0f, 0.0f, 2.0f), std::exp(-2.0f), 1.0e-4f);
    BOOST_CHECK_EQUAL(weight(Filter::Gaussian, 1.5f, 0.0f, 2.0f), 0.0f);

    // catmull-rom is radial, reaches zero two pixels out, is 2 at the centre and has a
    // negative lobe
    BOOST_CHECK_EQUAL(weight(Filter::CatmullRom, 0.0f, 0.0f, 4.0f), 2.0f);
    BOOST_CHECK_CLOSE(weight(Filter::CatmullRom, 0.5f, 0.0f, 4.0f), 1.125f, 1.0e-4f);
    BOOST_CHECK_SMALL(weight(Filter::CatmullRom, 1.0f, 0.0f, 4.0f), 1.0e-6f);
    BOOST_CHECK_CLOSE(weight(Filter::CatmullRom, 1.5f, 0.0f, 4.0f), -0.125f, 1.0e-3f);
    BOOST_CHECK_EQUAL(weight(Filter::CatmullRom, 2.5f, 0.0f, 4.0f), 0.0f);

    BOOST_CHECK_EQUAL(weight(Filter::Sinc, 0.0f, 0.0f, 4.0f), 1.0f);
    BOOST_CHECK_CLOSE(weight(Filter::Sinc, 0.5f, 0.0f, 4.0f), 2.0f / 3.14159265f, 1.0e-3f);
    BOOST_CHECK_SMALL(weight(Filter::Sinc, 1.0f, 0.0f, 4.0f), 1.0e-6f);
    BOOST_CHECK_EQUAL(weight(Filter::Sinc, 2.5f, 0.0f, 4.0f), 0.0f);
}

/**
 * Samples of one colour resolve to that colour wherever the filter reaches, and to full
 * coverage, including at the image's edges where part of the filter falls outside it.
 **/
BOOST_AUTO_TEST_CASE(film_uniform_test) {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(3, 3);
    v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(4, 3, sampling);

    const glm::vec3 colour(0.25f, 0.5f, 0.75f);
    for (unsigned int row = 0; row < 3; row++) {
        for (unsigned int column = 0; column < 4; column++) {
            for (const v3d::render::offline::Sampler::Sample & s : sampler.pixel(column, row)) {
                v3d::render::offline::Film::Sample sample;
                sample.raster = s.raster;
                sample.colour = colour;
                sample.opacity = glm::vec3(1.0f);
                sample.hit = true;
                film.add(sample);
            }
        }
    }

    for (unsigned int row = 0; row < 3; row++) {
        for (unsigned int column = 0; column < 4; column++) {
            const glm::vec3 resolved = film.colour(column, row);
            BOOST_CHECK_CLOSE(resolved.r, colour.r, 1.0e-4f);
            BOOST_CHECK_CLOSE(resolved.g, colour.g, 1.0e-4f);
            BOOST_CHECK_CLOSE(resolved.b, colour.b, 1.0e-4f);
            BOOST_CHECK_CLOSE(film.coverage(column, row), 1.0f, 1.0e-4f);
            BOOST_CHECK_CLOSE(film.opacity(column, row).g, 1.0f, 1.0e-4f);
        }
    }
}

/**
 * Half of a pixel's samples hitting is a coverage of a half under a box, the fraction the
 * coverage plane holds. A black miss weighs in, so the colour is premultiplied.
 **/
BOOST_AUTO_TEST_CASE(film_half_coverage_test) {
    const v3d::render::offline::Sampling sampling = box(2, 1.0f);
    v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(1, 1, sampling);

    const std::vector<v3d::render::offline::Sampler::Sample> samples = sampler.pixel(0, 0);
    BOOST_REQUIRE_EQUAL(samples.size(), 4u);
    for (const v3d::render::offline::Sampler::Sample & s : samples) {
        v3d::render::offline::Film::Sample sample;
        sample.raster = s.raster;
        sample.hit = s.raster.x < 0.5f;
        sample.colour = glm::vec3(sample.hit ? 1.0f : 0.0f);
        film.add(sample);
    }

    BOOST_CHECK_EQUAL(film.coverage(0, 0), 0.5f);
    BOOST_CHECK_EQUAL(film.colour(0, 0).r, 0.5f);
}

/**
 * One sample a pixel under a box one pixel wide resolves to the sample exactly, and the
 * sample is at the pixel's centre.
 **/
BOOST_AUTO_TEST_CASE(film_one_sample_is_exact_test) {
    const v3d::render::offline::Sampling sampling = box(1, 1.0f);
    v3d::render::offline::Sampler sampler(sampling);
    v3d::render::offline::Film film(3, 2, sampling);

    for (unsigned int row = 0; row < 2; row++) {
        for (unsigned int column = 0; column < 3; column++) {
            const std::vector<v3d::render::offline::Sampler::Sample> samples = sampler.pixel(column, row);
            BOOST_REQUIRE_EQUAL(samples.size(), 1u);
            BOOST_CHECK_EQUAL(samples[0].raster.x, static_cast<float>(column) + 0.5f);
            BOOST_CHECK_EQUAL(samples[0].raster.y, static_cast<float>(row) + 0.5f);

            v3d::render::offline::Film::Sample sample;
            sample.raster = samples[0].raster;
            // the middle pixel of the top row is a miss
            sample.hit = column != 1 || row != 0;
            if (sample.hit) {
                sample.colour = glm::vec3(0.1f * static_cast<float>(column), 0.3f, 0.7f / static_cast<float>(row + 1));
            }
            sample.depth = 2.0f + static_cast<float>(column);
            film.add(sample);
        }
    }

    v3d::render::offline::FrameBuffer frame(3, 2, 5);
    frame.clear(3, 99.0f);
    film.resolve(&frame, 0, 4, 3);

    BOOST_CHECK_EQUAL(frame.value(0, 2, 1), 0.1f * 2.0f);
    BOOST_CHECK_EQUAL(frame.value(1, 2, 1), 0.3f);
    BOOST_CHECK_EQUAL(frame.value(2, 2, 1), 0.7f / 2.0f);
    BOOST_CHECK_EQUAL(frame.value(3, 2, 1), 4.0f);
    BOOST_CHECK_EQUAL(frame.value(4, 2, 1), 1.0f);
    // the miss is black, uncovered, and keeps the depth its plane started with
    BOOST_CHECK_EQUAL(frame.value(0, 1, 0), 0.0f);
    BOOST_CHECK_EQUAL(frame.value(4, 1, 0), 0.0f);
    BOOST_CHECK_EQUAL(frame.value(3, 1, 0), 99.0f);
}

/**
 * A sample whose raster position is not finite, or lies far off the film on either side,
 * reaches no pixel: it adds no colour, no coverage and no depth.
 **/
BOOST_AUTO_TEST_CASE(film_sample_off_the_film_reaches_no_pixel_test) {
    const v3d::render::offline::Sampling sampling = box(1, 2.0f);
    v3d::render::offline::Film film(3, 2, sampling);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    const std::vector<glm::vec2> positions = {
        glm::vec2(nan, 0.5f), glm::vec2(0.5f, nan), glm::vec2(infinity, 0.5f), glm::vec2(0.5f, -infinity),
        glm::vec2(1.0e12f, 0.5f), glm::vec2(-1.0e12f, 0.5f), glm::vec2(0.5f, 1.0e12f), glm::vec2(0.5f, -1.0e12f),
    };
    for (const glm::vec2& position : positions) {
        v3d::render::offline::Film::Sample sample;
        sample.raster = position;
        sample.colour = glm::vec3(1.0f);
        sample.hit = true;
        sample.depth = 1.0f;
        film.add(sample);
    }
    for (unsigned int row = 0; row < 2; row++) {
        for (unsigned int column = 0; column < 3; column++) {
            float depth = 0.0f;
            BOOST_CHECK(!film.depth(column, row, &depth));
            BOOST_CHECK_EQUAL(film.coverage(column, row), 0.0f);
            BOOST_CHECK_EQUAL(film.colour(column, row).r, 0.0f);
        }
    }
}

/**
 * On a film whose filter width is NaN, a sample is filtered into no pixel. A hit still records
 * its depth in the pixel it lies in.
 **/
BOOST_AUTO_TEST_CASE(film_nan_filter_width_reaches_no_pixel_test) {
    const v3d::render::offline::Sampling sampling = box(1, std::numeric_limits<float>::quiet_NaN());
    v3d::render::offline::Film film(3, 2, sampling);
    v3d::render::offline::Film::Sample sample;
    sample.raster = glm::vec2(1.5f, 0.5f);
    sample.colour = glm::vec3(1.0f);
    sample.hit = true;
    sample.depth = 2.0f;
    film.add(sample);
    for (unsigned int row = 0; row < 2; row++) {
        for (unsigned int column = 0; column < 3; column++) {
            BOOST_CHECK_EQUAL(film.coverage(column, row), 0.0f);
            BOOST_CHECK_EQUAL(film.colour(column, row).r, 0.0f);
        }
    }
    float depth = 0.0f;
    BOOST_CHECK(film.depth(1, 0, &depth));
    BOOST_CHECK_EQUAL(depth, 2.0f);
}

/**
 * A pixel's samples are the same whatever order pixels are requested in and on a second
 * sampler, fall inside the pixel, and differ from one pass to the next.
 **/
BOOST_AUTO_TEST_CASE(film_sampler_is_deterministic_test) {
    v3d::render::offline::Sampling sampling;
    sampling.samples = glm::uvec2(4, 4);
    sampling.shutter = glm::vec2(0.0f, 0.5f);

    const v3d::render::offline::Sampler first(sampling);
    const std::vector<v3d::render::offline::Sampler::Sample> before = first.pixel(3, 5);
    first.pixel(0, 0);
    first.pixel(7, 2);
    const std::vector<v3d::render::offline::Sampler::Sample> after = first.pixel(3, 5);
    const v3d::render::offline::Sampler second(sampling);
    const std::vector<v3d::render::offline::Sampler::Sample> again = second.pixel(3, 5);

    BOOST_REQUIRE_EQUAL(before.size(), 16u);
    for (std::size_t i = 0; i < before.size(); i++) {
        BOOST_CHECK_EQUAL(before[i].raster.x, after[i].raster.x);
        BOOST_CHECK_EQUAL(before[i].raster.y, after[i].raster.y);
        BOOST_CHECK_EQUAL(before[i].time, again[i].time);
        BOOST_CHECK_EQUAL(before[i].lens.x, again[i].lens.x);

        BOOST_CHECK(before[i].raster.x >= 3.0f && before[i].raster.x < 4.0f);
        BOOST_CHECK(before[i].raster.y >= 5.0f && before[i].raster.y < 6.0f);
        BOOST_CHECK(before[i].time >= 0.0f && before[i].time <= 0.5f);
        BOOST_CHECK(glm::dot(before[i].lens, before[i].lens) <= 1.0001f);
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
        const unsigned int i = static_cast<unsigned int>((s.raster.x - 10.0f) * 3.0f);
        const unsigned int j = static_cast<unsigned int>((s.raster.y - 20.0f) * 2.0f);
        BOOST_REQUIRE(i < 3 && j < 2);
        cells[j * 3 + i]++;
    }
    for (unsigned int count : cells) {
        BOOST_CHECK_EQUAL(count, 1u);
    }
}
