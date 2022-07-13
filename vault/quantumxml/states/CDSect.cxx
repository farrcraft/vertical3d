#include "CDSect.h"

#include "../tree/CDSect.h"

using namespace XML;
using namespace FSM;

FSMState::CDSect::CDSect(FSMParser * fsm) : State<FSMParser>(fsm)
{
	fsm->addState("CDSect", this);
}

/*
[18]   	CDSect	   ::=   	 CDStart  CData  CDEnd
[19]   	CDStart	   ::=   	'<![CDATA['
[20]   	CData	   ::=   	(Char* - (Char* ']]>' Char*))
[21]   	CDEnd	   ::=   	']]>'
*/
bool FSMState::CDSect::run(void)
{
	if (!_fsm->stream()->compare(L"<![CDATA[", 9))
		_fsm->stream()->skipChars(9);
	else
		return false;
	_fsm->stream()->mark();
	while (_fsm->stream()->isChar() && _fsm->stream()->compare(L"]]>", 3))
		_fsm->stream()->nextChar();
	std::wstring str = _fsm->stream()->substr();

	boost::shared_ptr<XML::CDSect> cdsect(new XML::CDSect);
	cdsect->setData(str);
	_fsm->currentElement()->appendChild(cdsect);

	if (_fsm->stream()->current() == ']' && _fsm->stream()->peek() == ']' && _fsm->stream()->peek(2) == '>')
	{
		_fsm->stream()->skipChars(3);
		return true;
	}
	else
	{
		Exception e(Exception::UnknownScanError);
		e.message("Expected end of CDSect.");
		_fsm->error(e);
	}
	return false;
}
