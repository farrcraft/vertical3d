#ifndef INCLUDED_STATE_NOTATIONDECL
#define INCLUDED_STATE_NOTATIONDECL

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState
	{

		class NotationDecl : public FSM::State<FSMParser>
		{
			public:
				NotationDecl(FSMParser * fsm);

				bool run(void);

			protected:
				bool getID(ExternalID & extid);

		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_NOTATIONDECL
