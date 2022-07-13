#ifndef INCLUDED_STATE_ELEMENT
#define INCLUDED_STATE_ELEMENT

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState 
	{

		class Element : public FSM::State<FSMParser>
		{
			public:
				Element(FSMParser * fsm);

				bool run();

				bool content();
				bool CharData();
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_ELEMENT
