#include "Misc.h"

using namespace XML;
using namespace FSM;

FSMState::Misc::Misc(FSMParser * fsm) : State<FSMParser>(fsm)
{
	fsm->addState("Misc", this);
}

// [27] Misc			::= Comment | PI | S 
bool FSMState::Misc::run()
{
	if (_fsm->stream()->isBlank())
	{
		// optional ignorable whitespace
		_fsm->stream()->skipBlanks();
		return true;
	}

	if (_fsm->callState("Comment"))
		return true;

	if (_fsm->callState("PI"))
		return true;

	return false;
}
