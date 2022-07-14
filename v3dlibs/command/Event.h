/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

namespace v3d::command {

    /*
        there are events generated by key presses and releases
        e.g. CommandDirectory::dispatch(event_t)
        with event_t storing the event name and possibly type
        e.g. {"Keyboard::Q", MATCH_STATE_ON}
        binds loaded at init contain this information plus a command name

        storing the event information with the command it is bound to
        requires checking every command for a matching bind.
        the command table should store a mapping of events to command name
        binds for quick lookup.
    */

    class Event {
     public:
            typedef enum MatchState {
                MATCH_STATE_ON,
                MATCH_STATE_OFF,
                MATCH_STATE_ANY
            } MatchState;

            Event();
            explicit Event(const std::string & name, std::string ns = "", MatchState flag = MATCH_STATE_ANY);
            ~Event();

            std::string name(void) const;
            std::string ns(void) const;
            MatchState flag(void) const;

            void name(const std::string & n);
            void ns(const std::string & n);
            void flag(MatchState f);

     private:
            std::string namespace_;
            std::string name_;
            MatchState catchflag_;
    };

};  // namespace v3d::command
