/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <expected>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

namespace v3d::asset {

/**
 * One thread that runs jobs in the order they are posted, and hands each result back to the
 * thread that polls it.
 *
 * A job runs on the worker thread. It may read and decode files, through an asset::Manager or
 * otherwise. It must not touch anything the polling thread uses without a lock: no device
 * call, no ECS registry and no realtime registry.
 *
 * A completion runs on the polling thread, inside poll(), in the order the jobs were posted. It
 * receives the job's result, or the exception the job threw. The owner of a worker polls it
 * once a frame, from tick().
 *
 * Destroying a worker drops every job that has not started and every completion that has not
 * run. The job that is running finishes first, and the thread is joined.
 **/
class Worker final {
 public:
    /**
     * Start the thread.
     **/
    Worker();

    /**
     * Drop what has not started, let the running job finish, and join the thread.
     **/
    ~Worker();

    Worker(const Worker&) = delete;
    Worker& operator=(const Worker&) = delete;

    /**
     * Run a job on the worker thread, and its completion on the polling thread.
     *
     * @param job takes nothing and returns a value, which must be copyable. It runs on the worker
     *        thread
     * @param completion takes a std::expected holding the job's result or the exception it threw.
     *        It runs in poll()
     **/
    template <typename Job, typename Completion>
    void post(Job job, Completion completion) {
        using Result = std::invoke_result_t<Job&>;
        static_assert(!std::is_void_v<Result>, "a job returns what its completion receives");
        enqueue([job = std::move(job), completion = std::move(completion)]() mutable -> Done {
            std::expected<Result, std::exception_ptr> result = std::unexpected(std::exception_ptr());
            try {
                result = job();
            } catch (...) {
                result = std::unexpected(std::current_exception());
            }
            return [completion = std::move(completion), result = std::move(result)]() mutable {
                completion(std::move(result));
            };
        });
    }

    /**
     * Run the completion of every job that has finished, in the order the jobs were posted.
     *
     * A completion that throws stops the poll, and the throw reaches the caller. The completions
     * after it run on the next poll.
     *
     * @return how many completions ran
     **/
    std::size_t poll();

    /**
     * @return how many jobs have been posted and have not had their completion run
     **/
    std::size_t outstanding() const;

 private:
    /**
     * What runs on the polling thread once a job has finished.
     **/
    using Done = std::function<void()>;

    /**
     * A job as the thread runs it: it never throws, and returns what runs once it is done.
     **/
    using Task = std::function<Done()>;

    void enqueue(Task task);

    /**
     * The worker thread's loop: take a task, run it, and hold its completion.
     **/
    void run();

    mutable std::mutex mutex_;
    std::condition_variable wake_;
    std::deque<Task> tasks_;
    std::deque<Done> finished_;
    std::size_t outstanding_ = 0;
    bool stopping_ = false;
    std::thread thread_;
};

};  // namespace v3d::asset
