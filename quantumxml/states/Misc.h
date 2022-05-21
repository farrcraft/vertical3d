#ifndef INCLUDED_STATE_MISC
#define INCLUDED_STATE_MISC

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState 
	{

		class Misc : public FSM::State<FSMParser>
		{
			public:
				Misc(FSMParser * fsm);

				bool run();
/*
			private:
				bool _found;
*/
		};

	}; // end namespace FSMState
}; // end namespace XML


#endif // INCLUDED_STATE_MISC
