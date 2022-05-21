#include <boost/test/unit_test.hpp>

#include "../command/Command.h"

// test callback
bool callback(const v3D::CommandInfo & cmd, const std::string & param)
{
	std::string checkval("ok");
	if (param == checkval)
	{
		return true;
	}
	return false;
}

BOOST_AUTO_TEST_CASE( command_test )
{
	std::string name("cmdname");
	std::string scope("scope");
	v3D::CommandInfo info(name, scope);

	v3D::Command command(name, scope, callback);

	std::string othername("othercmd");
	v3D::CommandInfo otherInfo(othername, scope);
	BOOST_CHECK_EQUAL((command == info), true);
	BOOST_CHECK_EQUAL((command == otherInfo), false);

	bool result = command.exec("notok");
	BOOST_CHECK_EQUAL(result, false);
	result = command.exec("ok");
	BOOST_CHECK_EQUAL(result, true);
}