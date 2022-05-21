#ifndef INCLUDED_STATE_COMMENT
#define INCLUDED_STATE_COMMENT

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState 
	{

		class Comment : public FSM::State<FSMParser>
		{
			public:
				Comment(FSMParser * fsm);

				bool run(void);
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_COMMENT
