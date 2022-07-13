/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "../input/MouseDevice.h"
#include "../input/KeyboardDevice.h"
#include "../input/MouseEventListener.h"
#include "../input/KeyboardEventListener.h"
#include "../event/EventListener.h"

#include <boost/shared_ptr.hpp>

namespace v3d::input {

    /**
    * The InputEventAdapter provides the glue between libv3dcommand and libv3dinput, connecting the input device events to
    * the command library's generic event listener object type.
    */
    class InputEventAdapter :
        public MouseEventListener,
        public KeyboardEventListener {
     public:
            InputEventAdapter(boost::shared_ptr<KeyboardDevice> kb, boost::shared_ptr<MouseDevice> mouse);

            void connect(EventListener * target);

            // overrides from MouseEventListener
            void motion(unsigned int x, unsigned int y);
            void buttonPressed(unsigned int button);
            void buttonReleased(unsigned int button);
            // overrides from KeyboardEventListener base
            void keyPressed(const std::string & key);
            void keyReleased(const std::string & key);

     protected:
            void notify(const v3d::event::EventInfo & info);

     private:
            std::vector< EventListener * > targets_;
    };

};  // namespace v3d::input
