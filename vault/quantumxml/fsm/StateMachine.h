#ifndef INCLUDED_FSM_STATEMACHINE
#define INCLUDED_FSM_STATEMACHINE

#include <string>
#include <map>
#include <deque>

#include "StateBase.h"

namespace FSM
{

	/**
	 * A generic state machine
	 **/
	class StateMachine
	{
		public:
			/**
			 * Default Constructor.
			 **/
			StateMachine();
			/**
			 * Default Destructor.
			 **/
			~StateMachine();

			typedef std::map<std::string, StateBase *> value_type;

			/**
			 * Begin state machine execution.
			 * execution continues until there is no longer a next state in the chain.
			 * @param start the name of the first state to begin running
			 **/
			virtual void run(const std::string & start);

			/**
			 * Add a new named state to the FSM.
			 * @param name the name of the new state.
			 * @param state a pointer to thew new state object.
			 **/
			void addState(const std::string & name, StateBase * state);

			/**
			 * Get a pointer to the named state object.
			 * @param name the name of the requested state.
			 * @return a pointer to the requested state or null if it does not exist.
			 **/
			StateBase * getState(const std::string & name);

			/**
			 * Get the name of the active state.
			 * @return the active state name.
			 **/
			std::string activeStateName() const;
			/**
			 * Execute the named state.
			 * @param name the name of the state to execute.
			 * @return the execution result of the state.
			 **/
			bool callState(const std::string & name);

//			void push(void);
			/**
			 * @return the last state name on the stack *after* the pop is performed
			 **/
//			std::string pop(void);
			/**
			 * check if the named state is currently on the stack
			 **/
//			bool marked(const std::string & name);

			/**
			 * Get the name of the previous state.
			 * @return the previous state name.
			 **/
			std::string previousState();
			/**
			 * Look for state name in the chain of previously executed states.
			 * @param name the name of the state to look for.
			 * @return whether the named state has been executed at least once.
			 **/
			bool history(const std::string & name);

		private:
			StateBase * _current;
			StateBase * _next;
			std::string _activeStateName;
			value_type _states;
			std::deque<std::string> _stack;
			std::deque<std::string> _history;
	};

}; // end namespace FSM

#endif // INCLUDED_FSM_STATEMACHINE
