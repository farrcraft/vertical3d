#ifndef INCLUDED_STATE_PROCESSINGINSTRUCTION
#define INCLUDED_STATE_PROCESSINGINSTRUCTION

#include "../fsm/State.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState 
	{

		/**
		 *
		 **/
		class ProcessingInstruction : public FSM::State<FSMParser>
		{
			public:
				ProcessingInstruction(FSMParser * fsm);

				bool run(void);
				std::wstring PITarget(void) const;

			private:
		};

	}; // end namespace FSMState
}; // end namespace XML


#endif // INCLUDED_STATE_PROCESSINGINSTRUCTION
