#include "Document.h"

using namespace XML;
using namespace FSM;
//using namespace FSMState;

FSMState::Document::Document(FSMParser * fsm) : MultiState<FSMParser, FSMState::Document>(fsm)
{
	MultiState::functor_type documentAction(this, &FSMState::Document::document);
	MultiState::functor_type prologAction(this, &FSMState::Document::prolog);

	addState("document", documentAction);
	addState("prolog", prologAction);
}

// [1]	document		::= prolog element Misc*
bool FSMState::Document::document()
{
//	std::cout << "entered document state" << std::endl;

	if (!_fsm->callState("prolog"))
	{
		Exception e(Exception::UnknownScanError);
		e.message("error parsing document prolog!");
		_fsm->error(e);
		return false;
	}

	if (!_fsm->callState("element"))
	{
		Exception e(Exception::UnknownScanError);
		e.message("error parsing element!");
		_fsm->error(e);
		return false;
	}

	// epilog
	for (; _fsm->callState("Misc") != false; )
	{
	}

	if (!_fsm->stream()->end())
	{
		Exception e(Exception::WFC);
		e.message("content after document!");
		_fsm->error(e);
		return false;
	}
	return true;
}

// [22]	prolog			::= XMLDecl? Misc* (doctypedecl Misc*)?
bool FSMState::Document::prolog()
{
	_fsm->callState("XMLDecl");

	while (_fsm->callState("Misc") != false);

	if (_fsm->callState("doctypedecl"))
	{
		for (; _fsm->callState("Misc") != false; )
		{
		}
	}
	return true;
}
