#include <boost/test/unit_test.hpp>

#include "../event/EventInfo.h"

BOOST_AUTO_TEST_CASE( eventinfo_test )
{
    std::string name("myevent");
    v3D::EventInfo eventInfo(name, v3D::EventInfo::MATCH_STATE_ON);
    BOOST_CHECK_EQUAL((eventInfo.name() == name), true);
    BOOST_CHECK_EQUAL((eventInfo.match() == v3D::EventInfo::MATCH_STATE_ON), true);

    std::string name2("otherevent");
    std::string data("extradata");
    v3D::EventInfo eventInfo2(name2, v3D::EventInfo::MATCH_STATE_OFF, data);
    BOOST_CHECK_EQUAL((eventInfo2.name() == name2), true);
    BOOST_CHECK_EQUAL((eventInfo2.match() == v3D::EventInfo::MATCH_STATE_OFF), true);
    BOOST_CHECK_EQUAL((eventInfo2.data() == data), true);

    BOOST_CHECK_EQUAL((eventInfo == eventInfo2), false);
}