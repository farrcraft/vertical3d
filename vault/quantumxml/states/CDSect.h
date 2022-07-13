#ifndef INCLUDED_STATE_CDSECT
#define INCLUDED_STATE_CDSECT

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState 
	{

		class CDSect : public FSM::State<FSMParser>
		{
			public:
				CDSect(FSMParser * fsm);

				bool run(void);
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_CDSECT
