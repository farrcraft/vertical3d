#ifndef INCLUDED_STATE_ATTLISTDECL
#define INCLUDED_STATE_ATTLISTDECL

#include "../fsm/State.h"
#include "../FSMParser.h"

#include "../tree/AttList.h"

namespace XML
{
	namespace FSMState
	{

		/**
		 *
		 */
		class AttlistDecl : public FSM::State<FSMParser>
		{
			public:
				AttlistDecl(FSMParser * fsm);

				bool run(void);

				bool AttDef(AttList & attribs);
				bool AttType(XML::AttDef & def);
				bool DefaultDecl(XML::AttDef & def);
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_ATTLISTDECL
