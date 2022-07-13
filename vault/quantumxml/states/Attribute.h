#ifndef INCLUDED_STATE_ATTRIBUTE
#define INCLUDED_STATE_ATTRIBUTE

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState 
	{

		class Attribute : public FSM::State<FSMParser>
		{
			public:
				Attribute(FSMParser * fsm);

				bool run(void);

				bool AttValue(boost::shared_ptr<XML::Attribute> attr);
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_ATTRIBUTE
