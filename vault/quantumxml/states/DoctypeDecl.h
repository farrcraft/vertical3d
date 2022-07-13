#ifndef INCLUDED_STATE_DOCTYPEDECL
#define INCLUDED_STATE_DOCTYPEDECL

#include "../fsm/MultiState.h"
#include "../FSMParser.h"

namespace XML
{
	namespace FSMState
	{

		class DoctypeDecl : public FSM::MultiState<FSMParser, DoctypeDecl>
		{
			public:
				DoctypeDecl(FSMParser * fsm);

				bool doctypedecl(void);
//				bool externalID(void);
				bool markupdecl(void);
				bool DeclSep(void);
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_DOCTYPEDECL
