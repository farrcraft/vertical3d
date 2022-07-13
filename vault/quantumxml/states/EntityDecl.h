#ifndef INCLUDED_STATE_ENTITYDECL
#define INCLUDED_STATE_ENTITYDECL

#include "../fsm/State.h"
#include "../FSMParser.h"

#include "../tree/Entity.h"

namespace XML
{
	namespace FSMState
	{

		class EntityDecl : public FSM::State<FSMParser>
		{
			public:
				EntityDecl(FSMParser * fsm);

				bool run(void);

				bool GEDecl(Entity & entity);
				bool PEDecl(Entity & entity);
				bool EntityDef(Entity & entity);
				bool PEDef(Entity & entity);
				bool EntityValue(Entity & entity);

			private:
		};

	}; // end namespace FSMState
}; // end namespace XML

#endif // INCLUDED_STATE_ENTITYDECL
