#include <boost/test/unit_test.hpp>

#include "../command/CommandDirectory.h"

// test callback
bool dir_callback(const v3D::CommandInfo & cmd, const std::string & param)
{
	std::string checkval("ok");
	if (param == checkval)
	{
		return true;
	}
	return false;
}

BOOST_AUTO_TEST_CASE( commanddirectory_test )
{
	v3D::CommandDirectory directory;

	std::string name("cmdname");
	std::string scope("scope");
	v3D::CommandInfo info(name, scope);

	std::string eventName("myevent");
	v3D::EventInfo eventInfo(eventName, v3D::EventInfo::MATCH_STATE_ON);

	std::string param("bindparam");

	//v3D::Command command(name, scope, callback);

	boost::shared_ptr<v3D::Command> cmd(new v3D::Command(name, scope, dir_callback));
	directory.add(cmd);

	directory.bind(eventInfo, info, param);

	bool result = directory.remove(name,scope);
	BOOST_CHECK_EQUAL(result, true);
	result = directory.remove(name,scope);
	BOOST_CHECK_EQUAL(result, false);
	std::string otherName("othercmd");
	result = directory.remove(otherName, scope);
	BOOST_CHECK_EQUAL(result, false);

	directory.add(name, scope, dir_callback);

	std::string scopedName("scope::cmdname");
	v3D::Bind bind(eventInfo, info, param);
	v3D::Bind lookupBind = directory.lookup(scopedName);
	BOOST_CHECK_EQUAL((bind == lookupBind), true);

	v3D::Bind emptyBind(v3D::EventInfo("", v3D::EventInfo::MATCH_STATE_ANY), v3D::CommandInfo("", ""), "");
	v3D::Bind badBind = directory.lookup(name);
	BOOST_CHECK_EQUAL((badBind == emptyBind), true);

	result = directory.exec(info, param);
	BOOST_CHECK_EQUAL(result, true);

	v3D::CommandInfo otherInfo(otherName, scope);
	result = directory.exec(otherInfo, param);
	BOOST_CHECK_EQUAL(result, false);
}