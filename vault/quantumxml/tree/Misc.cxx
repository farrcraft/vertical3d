#include "Misc.h"

#include <sstream>

using namespace XML;

void Misc::position(MiscPosition mp)
{
	_position = mp;
}

Misc::MiscPosition Misc::position(void) const
{
	return _position;
}
