/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Event.h"

namespace v3d::command {
    Event::Event() : catchflag_(MATCH_STATE_ANY) {
    }

    Event::Event(const std::string& name, std::string ns, MatchState flag) {
        if (ns.size() == 0)
            ns = "__global__";

        namespace_ = ns;
        name_ = name;
        catchflag_ = flag;
    }

    Event::~Event() {
    }

    std::string Event::name(void) const {
        return name_;
    }

    std::string Event::ns(void) const {
        return namespace_;
    }

    Event::MatchState Event::flag(void) const {
        return catchflag_;
    }

    void Event::name(const std::string& n) {
        name_ = n;
    }

    void Event::ns(const std::string& n) {
        namespace_ = n;
    }

    void Event::flag(MatchState f) {
        catchflag_ = f;
    }

};  // namespace v3d::command
