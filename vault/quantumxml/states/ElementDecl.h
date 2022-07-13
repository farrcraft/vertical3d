#ifndef INCLUDED_STATE_ELEMENTDECL
#define INCLUDED_STATE_ELEMENTDECL

#include "../fsm/MultiState.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState
	{

		class ElementDecl : public FSM::MultiState<FSMParser, ElementDecl>
		{
			public:
				ElementDecl(FSMParser * fsm);

				bool elementdecl(void);
				bool contentspec(void);
				bool Mixed(void);
				bool children(void);

			private:
				ElemDecl * _elem;
		};

	}; // end namespace XML
}; // end namespace XML

#endif // INCLUDED_STATE_ELEMENTDECL
