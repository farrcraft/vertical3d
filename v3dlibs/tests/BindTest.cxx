#include <boost/test/unit_test.hpp>

#include "../command/Bind.h"

BOOST_AUTO_TEST_CASE( bind_test )
{
    std::string name("cmdname");
    std::string scope("scope");
    v3D::CommandInfo info(name, scope);

    std::string eventName("myevent");
    v3D::EventInfo eventInfo(eventName, v3D::EventInfo::MATCH_STATE_ON);

    std::string param("bindparam");
    v3D::Bind bind(eventInfo, info, param);

    std::string checkparam = bind.param();
    BOOST_CHECK_EQUAL((checkparam == param), true);

    std::string othername("otherevent");
    v3D::EventInfo otherEvent(othername, v3D::EventInfo::MATCH_STATE_OFF);
    v3D::EventInfo checkEvent = bind.event();
    BOOST_CHECK_EQUAL((checkEvent == eventInfo), true);

    v3D::CommandInfo checkCommand = bind.command();
    BOOST_CHECK_EQUAL((checkCommand == info), true);

    BOOST_CHECK_EQUAL((bind == eventInfo), true);
    BOOST_CHECK_EQUAL((bind == otherEvent), false);

    v3D::Bind checkBind(eventInfo, info, param);
    BOOST_CHECK_EQUAL((bind == checkBind), true);
    v3D::Bind otherBind(otherEvent, info, param);
    BOOST_CHECK_EQUAL((bind == otherBind), false);

    v3D::CommandInfo otherInfo(othername, scope);
    v3D::Bind newBind(eventInfo, otherInfo, param);
    BOOST_CHECK_EQUAL((bind == newBind), false);
}