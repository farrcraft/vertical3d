# ECS (Entity Component System) Design

The [Entt library](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system) is used as the ECS foundation.

Using the Odyssey implementation as a reference:

Odyssey::engine (game engine)
	Entt::registry
	Entt::dispatcher
	Systems # These don't derive from Entt
		Movement


A registry stores and manages entities (or identifiers) and components.

The registry emplace member function template creates, initializes and assigns to an entity the given component. 
It accepts a variable number of arguments to use to construct the component itself

registry.emplace<ComponentName>(Entity, ComponentConstructorParam, ComponentConstructorParam...)

Component examples
	Position
	Velocity
	Renderable
	Tickable


Observers

Registries can bind a listener to component and entity lifecycle events
	On Construct
	On Destruction
	On Update (Patch)

