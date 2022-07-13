#ifndef INCLUDED_FSM_MULTISTATE
#define INCLUDED_FSM_MULTISTATE

#include "State.h"
#include "Functor.h"

#include <map>

namespace FSM
{

	/**
	 * A State which can implement multiple states within a single class.
	 * Implementations of a MultiState should add each state they provide with a
	 * call to their addState method.
	 **/
	template <class T, class X>
	class MultiState : public State<T>
	{
		public:
			/**
			 * Default Constructor.
			 * @param fsm a pointer to the state machine.
			 **/
			MultiState(T * fsm) : State<T>(fsm)
			{
			}

			typedef TFunctor<X> functor_type;
			typedef std::map<std::string, functor_type> value_type;

			/**
			 * Execute the active state.
			 * @return the state execution's result.
			 **/
			virtual bool run(void)
			{
				std::string name = this->_fsm->activeStateName();
				typename value_type::iterator iter = _vtable.find(name);
				if (iter != _vtable.end())
					return _vtable[name].exec();

				return false;
			}

			//virtual std::string name(void) const;

			/**
			 * Register a new state with this object's internal state table and
			 * also with the FSM.
			 * @param name the name of the state being registered.
			 * @param func the function pointer to call when the state is active.
			 **/
			void addState(const std::string & name, functor_type func)
			{
				_vtable[name] = func;
				this->_fsm->addState(name, this);
			}

		private:
			value_type _vtable;
			std::string _current;
//			std::string _next;
	};

}; // end namespace FSM

#endif // INCLUDED_FSM_MULTISTATE
