/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Worker.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/UploadQueue.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

using v3d::asset::Worker;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::TextureTag;

namespace {

using Queue = v3d::render::realtime::UploadQueue<TextureTag, std::size_t>;

/**
 * Poll a worker until every job's completion has run, or ten seconds pass.
 *
 * @return whether everything completed
 **/
bool drain(Worker* worker) {
    const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (worker->outstanding() > 0) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        worker->poll();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

/**
 * Post a job for each size, under handles 0 upwards. Each job returns its size, which the
 * cases upload as that many bytes.
 **/
void post(Queue* queue, Worker* worker, const std::vector<std::size_t>& sizes) {
    for (std::size_t index = 0; index < sizes.size(); index++) {
        const std::size_t size = sizes[index];
        queue->post(worker, TextureHandle(static_cast<uint32_t>(index)), [size]() { return size; });
    }
}

BOOST_AUTO_TEST_SUITE(upload_queue_test)

/**
 * A poll uploads in the order the jobs were posted, and stops once the bytes reach the budget.
 * The first upload is made even when it alone is over the budget, so a large file cannot stall.
 **/
BOOST_AUTO_TEST_CASE(a_poll_uploads_in_order_up_to_the_budget) {
    Worker worker;
    Queue queue;
    post(&queue, &worker, {300, 50, 50, 10});
    BOOST_REQUIRE(drain(&worker));
    BOOST_CHECK_EQUAL(queue.arrived(), 4U);

    std::vector<uint32_t> uploaded;
    const auto waiting = [](const TextureHandle&) { return true; };
    const auto upload = [&uploaded](const TextureHandle& handle, std::size_t size) {
        uploaded.push_back(handle.id());
        return size;
    };
    const auto failed = [](const TextureHandle&, const std::string&) { BOOST_FAIL("nothing fails"); };

    BOOST_CHECK_EQUAL(queue.poll(100, waiting, upload, failed), 1U);
    BOOST_CHECK_EQUAL(queue.poll(100, waiting, upload, failed), 2U);
    BOOST_CHECK_EQUAL(queue.poll(100, waiting, upload, failed), 1U);
    BOOST_CHECK_EQUAL(queue.poll(100, waiting, upload, failed), 0U);
    BOOST_CHECK((uploaded == std::vector<uint32_t>{0, 1, 2, 3}));
}

/**
 * A handle the store is no longer waiting for is skipped. It is neither uploaded nor counted, and
 * the poll goes on to the next.
 **/
BOOST_AUTO_TEST_CASE(a_handle_no_longer_waiting_is_skipped) {
    Worker worker;
    Queue queue;
    post(&queue, &worker, {10, 10});
    BOOST_REQUIRE(drain(&worker));

    std::vector<uint32_t> uploaded;
    const std::size_t count = queue.poll(100, [](const TextureHandle& handle) { return handle.id() != 0; },
        [&uploaded](const TextureHandle& handle, std::size_t size) {
            uploaded.push_back(handle.id());
            return size;
        },
        [](const TextureHandle&, const std::string&) { BOOST_FAIL("nothing fails"); });
    BOOST_CHECK_EQUAL(count, 1U);
    BOOST_CHECK((uploaded == std::vector<uint32_t>{1}));
    BOOST_CHECK_EQUAL(queue.arrived(), 0U);
}

/**
 * A job that throws and an upload that throws are each reported once, with what was thrown,
 * and neither is counted as uploaded.
 **/
BOOST_AUTO_TEST_CASE(a_failed_job_or_upload_is_reported_once) {
    Worker worker;
    Queue queue;
    queue.post(&worker, TextureHandle(0U), []() -> std::size_t { throw std::runtime_error("unreadable"); });
    queue.post(&worker, TextureHandle(1U), []() { return std::size_t{10}; });
    queue.post(&worker, TextureHandle(2U), []() { return std::size_t{10}; });
    BOOST_REQUIRE(drain(&worker));

    std::vector<std::pair<uint32_t, std::string>> reports;
    const std::size_t count = queue.poll(100, [](const TextureHandle&) { return true; },
        [](const TextureHandle& handle, std::size_t size) {
            if (handle.id() == 1) {
                throw std::runtime_error("out of memory");
            }
            return size;
        },
        [&reports](const TextureHandle& handle, const std::string& reason) { reports.emplace_back(handle.id(), reason); });
    BOOST_CHECK_EQUAL(count, 1U);
    BOOST_REQUIRE_EQUAL(reports.size(), 2U);
    BOOST_CHECK_EQUAL(reports[0].first, 0U);
    BOOST_CHECK_EQUAL(reports[0].second, "unreadable");
    BOOST_CHECK_EQUAL(reports[1].first, 1U);
    BOOST_CHECK_EQUAL(reports[1].second, "out of memory");
}

/**
 * A result that arrives after its queue is gone is dropped, and polling the worker is safe.
 **/
BOOST_AUTO_TEST_CASE(a_result_after_the_queue_is_gone_is_dropped) {
    Worker worker;
    {
        Queue queue;
        post(&queue, &worker, {10});
    }
    BOOST_CHECK(drain(&worker));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
