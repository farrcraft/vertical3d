/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "StyleProperty.h"

using namespace Luxa;

StyleProperty::StyleProperty(const std::string & str) : align_(NULL_ALIGNMENT), name_(str)
{
}

StyleProperty::~StyleProperty()
{
}

StyleProperty::Alignment StyleProperty::align() const
{
	return align_;
}

void StyleProperty::align(Alignment a)
{
	align_ = a;
}

std::string StyleProperty::name() const
{
	return name_;
}

