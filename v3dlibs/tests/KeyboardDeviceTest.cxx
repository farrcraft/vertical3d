#include <boost/test/unit_test.hpp>

#include "../input/KeyboardDevice.h"
#include "../input/KeyboardEventListener.h"

/**
 * Need to mock up an event listener that we can use for testing a keyboard device
 */ 
class MockKeyboardEventListener : public v3D::KeyboardEventListener
{
	public:
		void keyPressed(const std::string & key) 
		{ 
			BOOST_CHECK_EQUAL((key == "q"), true);
		}
		void keyReleased(const std::string & key) 
		{ 
			BOOST_CHECK_EQUAL((key == "w"), true);
		}
		void notify(const v3D::EventInfo & e)
		{
		}
};

/**
 * The class we're testing is still abstract so we need to create a testable object
 */
class TestableKeyboardDevice : public v3D::KeyboardDevice
{
	public:
		bool tick(void)
		{
			return true;
		}
};

BOOST_AUTO_TEST_CASE( keyboardDevice_test )
{
	TestableKeyboardDevice device;
	MockKeyboardEventListener * listener = new MockKeyboardEventListener();
	device.addEventListener(listener, "mock");

	BOOST_CHECK_EQUAL(device.removeEventListener("bogus"), false);
	BOOST_CHECK_EQUAL(device.removeEventListener("mock"), true);

	device.addEventListener(listener, "mock");

	device.keyPressed("q");

	device.keyReleased("w");

	device.removeEventListener("mock");
	delete listener;

}