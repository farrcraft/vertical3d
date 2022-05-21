/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 */

#include "MenuItem.h"
#include "Menu.h"

using namespace Luxa;

MenuItem::MenuItem(ComponentManager * cm, const std::string & type, const std::string & label, 
				   const std::string & cmd, const std::string & scope, const std::string & param) : 
	Component(cm),
	label_(label), type_(type), command_(cmd), scope_(scope), param_(param)
{
}

void MenuItem::draw() const
{
}

void MenuItem::label(const std::string & str)
{
	label_ = str;
}

void MenuItem::submenu(boost::shared_ptr<Menu> sub)
{
	submenu_ = sub;
	sub->parent(menu_);
}

std::string MenuItem::label() const
{
	return label_;
}

boost::shared_ptr<Menu> MenuItem::submenu(void) const
{
	return submenu_;
}

std::string MenuItem::command() const
{
	return command_;
}

void MenuItem::menu(boost::weak_ptr<Menu> m)
{
	menu_ = m;
}

boost::weak_ptr<Menu> MenuItem::menu()
{
	return menu_;
}

std::string MenuItem::scope() const
{
	return scope_;
}

std::string MenuItem::param() const
{
	return param_;
}

std::string MenuItem::type() const
{
	return type_;
}
