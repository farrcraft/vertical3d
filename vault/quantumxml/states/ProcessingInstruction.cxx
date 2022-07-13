#include "ProcessingInstruction.h"

#include "../tree/ProcessingInstruction.h"

using namespace XML;
using namespace FSM;

FSMState::ProcessingInstruction::ProcessingInstruction(FSMParser * fsm) : State<FSMParser>(fsm)
{
	_fsm->addState("PI", this);
}

// [16] PI				::= '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>' 
bool FSMState::ProcessingInstruction::run(void)
{
	if (_fsm->stream()->current() == '<' && _fsm->stream()->peek() == '?')
	{
		_fsm->stream()->skipChars(2);
		std::wstring target = PITarget();

		if (target.empty())
		{
			Exception e(Exception::WFC);
			e.message("expected PITarget in PI.");
			_fsm->error(e);
		}

		// build xmlTree object
		boost::shared_ptr<XML::ProcessingInstruction> pi(new XML::ProcessingInstruction);
		pi->position(_fsm->getMiscLocation());
		pi->setTarget(target);
		// empty PI
		if (_fsm->stream()->current() == '?' && _fsm->stream()->peek() == '>')
		{
			_fsm->stream()->skipChars(2);
			pi->setData(L"");
			if (pi->position() == Misc::Body)
				_fsm->currentElement()->appendChild(pi);
			else
				_fsm->document()->addMisc(pi);
			return true;
		}
		// (S (Char* - (Char* '?>' Char*)))?
		if (!_fsm->stream()->isBlank())
		{
			Exception e(Exception::NoWhitespace);
			e.message("expected whitespace after PITarget.");
			_fsm->error(e);
		}
		else
			_fsm->stream()->skipBlanks();
		_fsm->stream()->mark();
		while (!(_fsm->stream()->current() == '?' && _fsm->stream()->peek() == '>'))
			_fsm->stream()->nextChar();
		// extract pi data
		std::wstring data = _fsm->stream()->substr();
		pi->setData(data);
		if (pi->position() == Misc::Body)
			_fsm->currentElement()->appendChild(pi);
		else
			_fsm->document()->addMisc(pi);

		_fsm->stream()->skipChars(2);
		return true;
	}
	return false;
}

// [17] PITarget		::= Name - (('X' | 'x') ('M' | 'm') ('L' | 'l')) 
std::wstring FSMState::ProcessingInstruction::PITarget(void) const
{
	// make sure next 3 chars aren't combination of 'xml'
	if ((_fsm->stream()->current() == 'X' || _fsm->stream()->current() == 'x')
	&& (_fsm->stream()->peek() == 'M' || _fsm->stream()->peek() == 'm')
	&& (_fsm->stream()->peek(2) == 'L' || _fsm->stream()->peek(2) == 'l'))
	{
		Exception e(Exception::IllegalChar);
		e.message("xml in PITarget.");
		_fsm->error(e);
	}
	std::wstring target = _fsm->stream()->name();

	return target;
}

