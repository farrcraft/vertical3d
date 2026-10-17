/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Worker.h"

#include <utility>

namespace v3d::asset {

/**
 **/
Worker::Worker() :
    thread_([this]() { run(); }) {
}

/**
 **/
Worker::~Worker() {
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        tasks_.clear();
    }
    wake_.notify_all();
    thread_.join();
}

/**
 **/
void Worker::enqueue(Task task) {
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        tasks_.push_back(std::move(task));
        outstanding_++;
    }
    wake_.notify_one();
}

/**
 **/
std::size_t Worker::poll() {
    std::size_t ran = 0;
    for (;;) {
        Done completion;
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            if (finished_.empty()) {
                return ran;
            }
            completion = std::move(finished_.front());
            finished_.pop_front();
            outstanding_--;
        }
        // run outside the lock, so a completion may post another job
        ran++;
        completion();
    }
}

/**
 **/
std::size_t Worker::outstanding() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return outstanding_;
}

/**
 **/
void Worker::run() {
    for (;;) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            wake_.wait(lock, [this]() { return stopping_ || !tasks_.empty(); });
            if (stopping_) {
                return;
            }
            task = std::move(tasks_.front());
            tasks_.pop_front();
        }
        Done completion = task();
        const std::lock_guard<std::mutex> lock(mutex_);
        finished_.push_back(std::move(completion));
    }
}

};  // namespace v3d::asset
