#ifndef INCLUDED_STATE_DOCUMENT
#define INCLUDED_STATE_DOCUMENT

#include "../fsm/MultiState.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState
	{

		class Document : public FSM::MultiState<FSMParser, Document>
		{
			public:
				Document(FSMParser * fsm);

				bool document();
				bool prolog();
		};

	}; // end namespace XML
}; // end namespace XML

#endif // INCLUDED_STATE_DOCUMENT
