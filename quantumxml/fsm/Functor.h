#ifndef INCLUDED_FSM_FUNCTOR
#define INCLUDED_FSM_FUNCTOR

#include <string>

namespace FSM
{

	/**
	 * A Function Object.
	 * This is the Abstract Base Class for FSM function objects.
	 **/
	class Functor
	{
		public:
			/**
			 * Default Constructor.
			 **/
			Functor() { }
			/**
			 * Execute the functor. 
			 * @return result of the functor's execution.
			 **/
			virtual bool exec(void) = 0;
	};

	/**
	 * Templated Functor class.
	 **/
	template<class TC>
	class TFunctor : public Functor
	{
		public:
			typedef bool (TC::*functor_type)(void);

			/**
			 * Default Constructor.
			 **/
			TFunctor() : _obj(0), _func(0) { }
			/**
			 * Copy Constructor.
			 * @param TF the templated functor to initialize from.
			 **/
			TFunctor(const TFunctor<TC> & TF) : _obj(TF._obj), _func(TF._func) { }
			/**
			 * Constructor.
			 * @param obj the functor object.
			 * @param func the functor's callback method.
			 **/
			TFunctor(TC * obj, functor_type func)
			{
				_obj = obj;
				_func = func;
			}

			/**
			 * Execute the functor's callback method.
			 * @return the functor's execution result.
			 **/
			virtual bool exec(void)
			{
				return (*_obj.*_func)();
			}

		private:
			TC * _obj;
			functor_type _func;
	};

}; // end namespace FSM

#endif // INCLUDED_FSM_FUNCTOR
