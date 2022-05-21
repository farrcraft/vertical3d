#ifndef INCLUDED_FSM_STATEBASE
#define INCLUDED_FSM_STATEBASE

#include <string>

namespace FSM
{

	/**
	 * The Abstract Base Class for derived State classes.
	 * Reference counting is also implemented here. This is necessary so that
	 * the FSM can properly cleanup MultiState derived states.
	 **/
	class StateBase
	{
		public:
			StateBase();
			virtual ~StateBase();

			/**
			 * execute the state
			 * @return a pointer to the next execution state 
			 **/
			virtual bool run(void) = 0;

			/**
			 * Increase the reference count for the state object.
			 * @return the new reference count.
			 **/
			unsigned int addref(void);
			/**
			 * Decrease the reference count for the state object.
			 * @return the new reference count.
			 **/
			unsigned int release(void);

		private:
			unsigned int _refs;
	};

}; // end namespace FSM

#endif // INCLUDED_FSM_STATEBASE
