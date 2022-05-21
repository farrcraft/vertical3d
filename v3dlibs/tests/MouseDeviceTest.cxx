#include <boost/test/unit_test.hpp>

#include "../input/MouseDevice.h"
#include "../input/MouseEventListener.h"

/**
 * Need to mock up an event listener that we can use for testing a mouse device
 */ 
class MockMouseEventListener : public v3D::MouseEventListener
{
	public:
		void motion(unsigned int x, unsigned int y)
		{
			BOOST_CHECK_EQUAL(x, 3);
			BOOST_CHECK_EQUAL(y, 0);
		}
		void buttonPressed(unsigned int button)
		{
			BOOST_CHECK_EQUAL(button, 1);
		}
		void buttonReleased(unsigned int button)
		{
			BOOST_CHECK_EQUAL(button, 2);
		}
		void notify(const v3D::EventInfo & e)
		{
		}
};

/**
 * The class we're testing is still abstract so we need to create a testable object
 */
class TestableMouseDevice : public v3D::MouseDevice
{
	public:
		bool tick(void)
		{
			return true;
		}
};

BOOST_AUTO_TEST_CASE( mouseDevice_test )
{
	TestableMouseDevice device;
	MockMouseEventListener * listener = new MockMouseEventListener();
	device.addEventListener(listener, "mock");

	BOOST_CHECK_EQUAL(device.removeEventListener("bogus"), false);
	BOOST_CHECK_EQUAL(device.removeEventListener("mock"), true);

	device.addEventListener(listener, "mock");

	device.buttonPressed(1);

	device.buttonReleased(2);

	device.motion(3, 0);

	device.removeEventListener("mock");
	delete listener;
}