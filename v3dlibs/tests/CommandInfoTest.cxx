#include <boost/test/unit_test.hpp>

#include "../command/CommandInfo.h"

BOOST_AUTO_TEST_CASE( commandinfo_test )
{
    std::string name("cmdname");
    std::string scope("scope");
    v3D::CommandInfo info(name, scope);

    BOOST_CHECK_EQUAL((info.name() == name), true);
    BOOST_CHECK_EQUAL((info.scope() == scope), true);

    std::string fullname("scope::cmdname");
    v3D::CommandInfo info2(fullname);
    BOOST_CHECK_EQUAL((info2.name() == name), true);
    BOOST_CHECK_EQUAL((info2.scope() == scope), true);

    BOOST_CHECK_EQUAL((info == info2), true);

    std::string name2("othercmd");
    v3D::CommandInfo info3(name2, scope);
    BOOST_CHECK_EQUAL((info == info3), false);

    v3D::CommandInfo info4(scope);
    std::string name3;
    //BOOST_CHECK_EQUAL((info4.name() == name3), true);
    BOOST_CHECK_EQUAL((info4.scope() == scope), true);
}