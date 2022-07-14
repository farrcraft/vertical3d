/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

namespace v3d::event {

    /**
     * The EventInfo class stores information about an event. It is composed of an 
     * event name and a state flag.
     */
    class EventInfo {
     public:
            /**
             * catch flag enumeration
             **/
            typedef enum {
                MATCH_STATE_ON,  /**< match key/button press state **/
                MATCH_STATE_OFF,  /**< match key/button release state **/
                MATCH_STATE_ANY,  /**< match any changed state **/
                MATCH_STATE_NONE  /**< null match state matches nothing **/
            } MatchState;

            /**
             * Default constructor
             * @param name the name of the event
             * @param match the state of the event
             */
            EventInfo(const std::string & name, MatchState match);
            /**
             * Constructor
             * @param name the name of the event
             * @param match the state of the event
             * @param data the data associated with the event
             */
            EventInfo(const std::string & name, MatchState match, const std::string & data);

            /**
             * Comparison operator. If either of the events has a match state of any then only the names
             * must match for the events to match. A match state of none will never match another event.
             * @param e the event to compare against this one.
             * @return whether the two events match.
             */
            bool operator == (const EventInfo & e) const;

            /**
             * Get the name of the event
             * @return the event name
             */
            std::string name() const;
            /**
             * Get the match state of the event
             * @return the match state
             */
            MatchState match() const;
            /**
             * Get the optional data associated with the event
             * @return the event data
             */
            std::string data() const;

     private:
            std::string name_;
            std::string data_;
            MatchState match_;
    };

};  // namespace v3d::event
