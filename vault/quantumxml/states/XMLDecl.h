#ifndef INCLUDED_STATE_XMLDECL
#define INCLUDED_STATE_XMLDECL

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{

	namespace FSMState
	{

		class XMLDecl : public FSM::State<FSMParser>
		{
			public:
				XMLDecl(FSMParser * fsm);

				bool run(void);

				bool version(void);
				bool encoding(void);
				bool standalone(void);
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_XMLDECL
