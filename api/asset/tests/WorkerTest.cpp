/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Worker.h>

#include <atomic>
#include <chrono>
#include <exception>
#include <expected>
#include <latch>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::asset::Worker;

namespace {

/**
 * Poll until every posted job has had its completion run, or ten seconds pass.
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

BOOST_AUTO_TEST_SUITE(worker_test)

/**
 * A job runs on the worker thread, and its completion on the thread that polls, in the order
 * the jobs were posted.
 **/
BOOST_AUTO_TEST_CASE(completions_run_in_order_on_the_polling_thread) {
    Worker worker;
    const std::thread::id polling = std::this_thread::get_id();
    std::vector<int> results;
    std::atomic<int> onPollingThread{ 0 };
    std::atomic<int> jobsOnPollingThread{ 0 };

    for (int value = 1; value <= 5; value++) {
        worker.post(
            [value, polling, &jobsOnPollingThread]() {
                if (std::this_thread::get_id() == polling) {
                    jobsOnPollingThread++;
                }
                return value;
            },
            [&results, &onPollingThread, polling](std::expected<int, std::exception_ptr> result) {
                if (std::this_thread::get_id() == polling) {
                    onPollingThread++;
                }
                results.push_back(result.value());
            });
    }

    BOOST_REQUIRE(drain(&worker));
    BOOST_CHECK((results == std::vector<int>{ 1, 2, 3, 4, 5 }));
    BOOST_CHECK_EQUAL(onPollingThread.load(), 5);
    BOOST_CHECK_EQUAL(jobsOnPollingThread.load(), 0);
}

/**
 * A finished job's completion waits for a poll, so nothing reaches the polling thread's state
 * between polls.
 **/
BOOST_AUTO_TEST_CASE(a_finished_job_completes_only_when_polled) {
    Worker worker;
    std::latch finished(1);
    bool completed = false;

    worker.post([&finished]() { finished.count_down(); return 7; },
        [&completed](std::expected<int, std::exception_ptr>) { completed = true; });
    finished.wait();
    // the job has returned, and its completion is held until the poll
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    BOOST_CHECK(!completed);
    BOOST_CHECK_EQUAL(worker.outstanding(), 1U);
    BOOST_REQUIRE(drain(&worker));
    BOOST_CHECK(completed);
}

/**
 * A job that throws hands its exception to its completion, and the worker goes on to the next.
 **/
BOOST_AUTO_TEST_CASE(a_throwing_job_hands_its_exception_to_its_completion) {
    Worker worker;
    std::string message;
    int next = 0;

    worker.post([]() -> int { throw std::runtime_error("no such file"); },
        [&message](std::expected<int, std::exception_ptr> result) {
            BOOST_REQUIRE(!result.has_value());
            try {
                std::rethrow_exception(result.error());
            } catch (const std::runtime_error& error) {
                message = error.what();
            }
        });
    worker.post([]() { return 3; },
        [&next](std::expected<int, std::exception_ptr> result) { next = result.value(); });

    BOOST_REQUIRE(drain(&worker));
    BOOST_CHECK_EQUAL(message, "no such file");
    BOOST_CHECK_EQUAL(next, 3);
}

/**
 * A completion that throws stops the poll and reaches the caller. The completions after it run
 * on the next poll.
 **/
BOOST_AUTO_TEST_CASE(a_throwing_completion_leaves_the_rest_for_the_next_poll) {
    Worker worker;
    std::latch finished(2);
    bool second = false;

    worker.post([&finished]() { finished.count_down(); return 1; },
        [](std::expected<int, std::exception_ptr>) { throw std::logic_error("completion failed"); });
    worker.post([&finished]() { finished.count_down(); return 2; },
        [&second](std::expected<int, std::exception_ptr>) { second = true; });
    finished.wait();
    // the second job starts after the first job's completion is queued, so the first is waiting
    BOOST_CHECK_THROW(worker.poll(), std::logic_error);

    BOOST_CHECK(!second);
    BOOST_REQUIRE(drain(&worker));
    BOOST_CHECK(second);
}

/**
 * A completion may post another job, which completes on a later poll.
 **/
BOOST_AUTO_TEST_CASE(a_completion_may_post_another_job) {
    Worker worker;
    int last = 0;

    worker.post([]() { return 1; }, [&worker, &last](std::expected<int, std::exception_ptr> first) {
        worker.post([value = first.value()]() { return value + 1; },
            [&last](std::expected<int, std::exception_ptr> second) { last = second.value(); });
    });

    BOOST_REQUIRE(drain(&worker));
    BOOST_CHECK_EQUAL(last, 2);
}

/**
 * Destroying a worker lets the running job finish, drops the jobs that have not started, and
 * runs no completion.
 **/
BOOST_AUTO_TEST_CASE(destroying_a_worker_drops_what_has_not_started) {
    std::unique_ptr<Worker> worker = std::make_unique<Worker>();
    std::latch started(1);
    std::latch gate(1);
    std::atomic<bool> firstFinished{ false };
    std::atomic<bool> secondRan{ false };
    bool completed = false;

    worker->post([&]() { started.count_down(); gate.wait(); firstFinished = true; return 1; },
        [&completed](std::expected<int, std::exception_ptr>) { completed = true; });
    worker->post([&secondRan]() { secondRan = true; return 2; },
        [&completed](std::expected<int, std::exception_ptr>) { completed = true; });
    started.wait();

    // the destructor drops the second job before it joins, and the join waits for the first.
    // The gate opens once the destructor is well under way.
    std::thread opener([&gate]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        gate.count_down();
    });
    worker.reset();
    opener.join();

    BOOST_CHECK(firstFinished.load());
    BOOST_CHECK(!secondRan.load());
    BOOST_CHECK(!completed);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
