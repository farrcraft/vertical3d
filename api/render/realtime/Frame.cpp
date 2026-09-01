/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Frame.h"

#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

    /**
     **/
    Frame::Frame(boost::shared_ptr<Context> context) :
        context_(context) {
    }

    /**
     **/
    boost::shared_ptr<Pass> Frame::pass(const std::string& name) {
        for (const boost::shared_ptr<Pass>& pass : passes_) {
            if (pass->name() == name) {
                return pass;
            }
        }

        boost::shared_ptr<Pass> pass = boost::make_shared<Pass>(name);
        passes_.push_back(pass);
        return pass;
    }

    /**
     **/
    const std::vector<boost::shared_ptr<Pass>>& Frame::passes() const noexcept {
        return passes_;
    }

    /**
     **/
    boost::shared_ptr<Context> Frame::context() const noexcept {
        return context_;
    }

    /**
     **/
    void Frame::reset() noexcept {
        for (const boost::shared_ptr<Pass>& pass : passes_) {
            pass->reset();
        }
    }

};  // namespace v3d::render::realtime
