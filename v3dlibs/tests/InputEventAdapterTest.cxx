#include <boost/test/unit_test.hpp>

#include "../gui/InputEventAdapter.h"

// concrete device implementations needed for creating adapters
class TestableKeyboardDevice : public v3D::KeyboardDevice
{
    public:
        bool tick(void)
        {
            return true;
        }
};

class TestableMouseDevice : public v3D::MouseDevice
{
    public:
        bool tick(void)
        {
            return true;
        }
};

// a series of event listeners which each assert that a single event happens
class MouseButtonPressEventListener : public v3D::EventListener
{
    public:
        void notify(const v3D::EventInfo & e)
        {
            std::string eventName = "mouse::button_1";
            v3D::EventInfo checkInfo(eventName, v3D::EventInfo::MATCH_STATE_ON);
            BOOST_CHECK_EQUAL((e == checkInfo), true);
        }
};

class MouseButtonReleaseEventListener : public v3D::EventListener
{
    public:
        void notify(const v3D::EventInfo & e)
        {
            std::string eventName = "mouse::button_2";
            v3D::EventInfo checkInfo(eventName, v3D::EventInfo::MATCH_STATE_OFF);
            BOOST_CHECK_EQUAL((e == checkInfo), true);
        }
};

class MouseMotionEventListener : public v3D::EventListener
{
    public:
        void notify(const v3D::EventInfo & e)
        {
            std::string eventName = "mouse::motion";
            v3D::EventInfo checkInfo(eventName, v3D::EventInfo::MATCH_STATE_ANY);
            BOOST_CHECK_EQUAL((e == checkInfo), true);
        }
};

class KeyPressEventListener : public v3D::EventListener
{
    public:
        void notify(const v3D::EventInfo & e)
        {
            std::string eventName = "q";
            v3D::EventInfo checkInfo(eventName, v3D::EventInfo::MATCH_STATE_ON);
            BOOST_CHECK_EQUAL((e == checkInfo), true);
        }
};

class KeyReleaseEventListener : public v3D::EventListener
{
    public:
        void notify(const v3D::EventInfo & e)
        {
            std::string eventName = "w";
            v3D::EventInfo checkInfo(eventName, v3D::EventInfo::MATCH_STATE_OFF);
            BOOST_CHECK_EQUAL((e == checkInfo), true);
        }
};


// initiate the tests
BOOST_AUTO_TEST_CASE( inputEventAdapter_test )
{
    boost::shared_ptr<v3D::KeyboardDevice> keyboard(new TestableKeyboardDevice());
    boost::shared_ptr<v3D::MouseDevice> mouse(new TestableMouseDevice());
    boost::shared_ptr<v3D::MouseDevice> empty_mouse_ptr;
    boost::shared_ptr<v3D::KeyboardDevice> empty_keyboard_ptr;

    // create an adapter to use for testing
    v3D::InputEventAdapter adapter(empty_keyboard_ptr, mouse);
    MouseButtonPressEventListener mouseButtonListener;
    // connect the listener
    adapter.connect(&mouseButtonListener);

    // this should fire the buttonPressed method in the adapter which in turn calls notify on our listener
    mouse->buttonPressed(1);

    // test dispatching mouse button release events
    v3D::InputEventAdapter adapter2(keyboard, mouse);
    MouseButtonReleaseEventListener mouseReleaseListener;
    adapter2.connect(&mouseReleaseListener);
    // NOTE: devices bind to listeners by name and since every adapter uses the same internal listener name
    // when binding devices to an adapter, those devices previous adapter listeners (if they had one bound)
    // will be replaced by the new adapter. This means only our current listener is called and not any 
    // listeners bound to previous adapters.
    mouse->buttonReleased(2);

    // test dispatching mouse move events
    v3D::InputEventAdapter adapter3(keyboard, mouse);
    MouseMotionEventListener mouseMotionListener;
    adapter3.connect(&mouseMotionListener);
    mouse->motion(2, 3);

    // test dispatching key press events
    v3D::InputEventAdapter adapter4(keyboard, empty_mouse_ptr);
    KeyPressEventListener keypressListener;
    adapter4.connect(&keypressListener);
    keyboard->keyPressed("q");
    
    // test dispatching key release events
    v3D::InputEventAdapter adapter5(keyboard, mouse);
    KeyReleaseEventListener keyreleaseListener;
    adapter5.connect(&keyreleaseListener);
    keyboard->keyReleased("w");
}