#ifndef INCLUDED_FSM_STATE
#define INCLUDED_FSM_STATE

#include "StateBase.h"

namespace FSM
{

	/**
	 * A single state of execution within the Finite State Machine.
	 * 
	 **/
	template <class T>
	class State : public StateBase
	{
		public:
			/**
			 * Default Constructor.
			 * @param fsm a pointer to the StateMachine class controlling this state.
			 **/
			State(T * fsm) : _fsm(fsm)
			{
			}

			/**
			 * Execute the state.
			 * @return a pointer to the next execution state 
			 **/
			virtual bool run(void)
			{
				return false;
			}

			/**
			 * @return the name of the state
			 **/
			//virtual std::string name(void) const;

		protected:
			T * _fsm;
/*
		private:
			std::string _name;
*/
		};

}; // end namespace FSM

#endif // INCLUDED_FSM_STATE
