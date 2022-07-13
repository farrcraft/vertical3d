#include "Comment.h"

#include "../tree/Comment.h"

using namespace XML;
using namespace FSM;

FSMState::Comment::Comment(FSMParser * fsm) : State<FSMParser>(fsm)
{
	fsm->addState("Comment", this);
}

// [15] Comment			::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->' 
bool FSMState::Comment::run(void)
{
	if (!_fsm->stream()->compare(L"<!--", 4))
	{
		_fsm->stream()->skipChars(4);
		std::wstring str;
		_fsm->stream()->mark();
		while (_fsm->stream()->isChar() && !((_fsm->stream()->current() == '-') && 
			   (_fsm->stream()->peek() == '-') && (_fsm->stream()->peek(2) == '>')))
		{
			if (_fsm->stream()->current() == '-' && _fsm->stream()->peek() == '-'
			 && _fsm->stream()->peek(2) != '>')
			{
				Exception e(Exception::IllegalChar);
				e.message("-- in Comment.");
				_fsm->error(e);
				return false;
			}
			_fsm->stream()->nextChar();
		}
		if (!_fsm->stream()->compare(L"-->", 3))
		{
			str = _fsm->stream()->substr();
			_fsm->stream()->skipChars(3);
			// build xmlTree object
			boost::shared_ptr<XML::Comment> cmnt(new XML::Comment);
			cmnt->position(_fsm->getMiscLocation());
			cmnt->comment(str);
			if (cmnt->position() == Misc::Body)
				_fsm->currentElement()->appendChild(cmnt);
			else
				_fsm->document()->addMisc(cmnt);

			return true;
		}
		else
		{
			if (!_fsm->stream()->isChar())
			{
				Exception e(Exception::IllegalChar);
				e.message("Illegal character in Comment.");
				_fsm->error(e);
			}
			else
			{
				Exception e(Exception::UnknownScanError);
				e.message("expected --> in Comment.");
				_fsm->error(e);
			}
		}
	}
	return false;
}
