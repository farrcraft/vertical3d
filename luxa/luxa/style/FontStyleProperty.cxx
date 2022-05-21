/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "FontStyleProperty.h"

using namespace Luxa;

FontStyleProperty::FontStyleProperty(const std::string & name, const std::string & src) : StyleProperty(name), source_(src), italics_(false), bold_(false)
{
}

FontStyleProperty::~FontStyleProperty()
{
}

bool FontStyleProperty::italics() const
{
	return italics_;
}

bool FontStyleProperty::bold() const
{
	return bold_;
}

std::string FontStyleProperty::face() const
{
	return face_;
}

unsigned int FontStyleProperty::size() const
{
	return size_;
}

std::string FontStyleProperty::source() const
{
	return source_;
}
