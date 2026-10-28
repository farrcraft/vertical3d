/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Worker.h>
#include <api/render/realtime/Handle.h>

#include <cstddef>
#include <deque>
#include <exception>
#include <expected>
#include <string>
#include <utility>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

namespace v3d::render::realtime {

/**
 * What a store's requests wait in between the worker and the device. A job decodes a file on
 * the worker thread. Its result arrives here when the worker is polled, and poll() uploads what
 * has arrived, in the order it was posted, up to a byte budget.
 *
 * Every store that streams follows one rule for a request that fails. The queue reports the
 * failure once, through the store's failed callback. The store logs it and frees the path, so
 * that asking for it again reads the file again. A texture's handle draws white until it is
 * released, and a mesh's handle resolves to nothing.
 *
 * @param Tag the handle's tag
 * @param Value what a job returns, which must be copyable
 **/
template <typename Tag, typename Value>
class UploadQueue final {
 public:
    UploadQueue() : inbox_(boost::make_shared<Inbox>()) {
    }

    UploadQueue(const UploadQueue&) = delete;
    UploadQueue& operator=(const UploadQueue&) = delete;

    /**
     * Run a job on the worker, and hold its result for poll().
     *
     * A completion holds the queue weakly, so a result that arrives after the queue is gone is
     * dropped.
     *
     * @param job takes nothing and returns a Value. It runs on the worker thread
     * @throw whatever the worker's post() throws, and then nothing is queued
     **/
    template <typename Job>
    void post(asset::Worker* worker, const Handle<Tag>& handle, Job job) {
        worker->post(std::move(job), [inbox = boost::weak_ptr<Inbox>(inbox_), handle](Result result) {
            const boost::shared_ptr<Inbox> held = inbox.lock();
            if (held) {
                held->push_back(Arrival{handle, std::move(result)});
            }
        });
    }

    /**
     * Upload what has arrived. At least one value is uploaded when any is waiting, and the poll
     * stops once the bytes uploaded reach the budget. The rest wait for the next poll.
     *
     * @param budget the bytes to upload before stopping
     * @param waiting takes a handle, and says whether the store is still waiting for its value.
     *        A handle released, or filled another way, since its job was posted is skipped
     * @param upload takes a handle and its value, uploads it, and returns the bytes that cost. It
     *        throws a std::exception when the value cannot be uploaded
     * @param failed takes a handle and what went wrong, when the job threw or the upload did
     * @return how many values were uploaded
     **/
    template <typename Waiting, typename Upload, typename Failed>
    std::size_t poll(std::size_t budget, Waiting waiting, Upload upload, Failed failed) {
        std::size_t count = 0;
        std::size_t spent = 0;
        while (!inbox_->empty() && (count == 0 || spent < budget)) {
            Arrival arrival = std::move(inbox_->front());
            inbox_->pop_front();
            if (!waiting(arrival.handle)) {
                continue;
            }
            if (!arrival.result) {
                failed(arrival.handle, describe(arrival.result.error()));
                continue;
            }
            try {
                spent += upload(arrival.handle, *arrival.result);
            } catch (const std::exception& error) {
                failed(arrival.handle, std::string(error.what()));
                continue;
            }
            count++;
        }
        return count;
    }

    /**
     * @return how many results have arrived and wait for a poll
     **/
    std::size_t arrived() const noexcept {
        return inbox_->size();
    }

 private:
    using Result = std::expected<Value, std::exception_ptr>;

    struct Arrival final {
        Handle<Tag> handle;
        Result result;
    };
    using Inbox = std::deque<Arrival>;

    /**
     * What an exception says, for the line a failed request is reported on.
     **/
    static std::string describe(const std::exception_ptr& error) {
        if (!error) {
            return "an unknown error";
        }
        try {
            std::rethrow_exception(error);
        } catch (const std::exception& thrown) {
            return thrown.what();
        } catch (...) {
            return "an error that is not a std::exception";
        }
    }

    boost::shared_ptr<Inbox> inbox_;
};

};  // namespace v3d::render::realtime
