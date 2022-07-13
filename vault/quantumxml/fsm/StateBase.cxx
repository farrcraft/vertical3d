#include "StateBase.h"

using namespace FSM;

StateBase::StateBase() : _refs(0)
{
}

StateBase::~StateBase()
{
}

unsigned int StateBase::addref(void)
{
	return ++_refs;
}

unsigned int StateBase::release(void)
{
	//assert(_refs > 0);
	return --_refs;
}
