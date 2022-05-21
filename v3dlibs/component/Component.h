/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include "../3dtypes/Vector3.h"
#include "../3dtypes/Matrix4.h"

/*
Supporting library structures might look like:

libv3dcomponent.lib
namespaces:
v3D::
	 Component::
classes:
				Node		- a single base component type
				Aggregate	- a component built from multiple other components
				Collection	- a group of multiple components

libv3dcomponents.lib
dependencies:
	libv3dcomponent
	libv3dtypes
classes:
	Position3Component - a component node that contains a single vector3 position value
	TransformComponent - a component node that contains a matrix4 transformation

additional types built from components:
Camera
Light
Shape
*/

namespace v3d::component
{


		class Node
		{
			public:
				virtual ~Node() { }

			private:
				std::string name_;
		};

		class Aggregate : public Node
		{
		};

		class Collection : public Node
		{
		};

		/**
		 * a position contains 3d position information
		 */
		class Position3 : public Node
		{
			public:
				Vector3 operator * () const;

			private:
				Vector3 position_;
		};


		/**
		 * a transform contains translation, rotation, & scaling information
		 */
		class Transform : public Node
		{
			public:
				Matrix4 operator * () const;

			private:
				Matrix4 transform_;
		};


	class Camera : public Component::Aggregate
	{
		public:
			Camera()
			{
				Component::Position3 eye;
				addComponent(eye);
			}
	};

	class PolyShape : public Component::Aggregate
	{
		public:
			PolyShape()
			{
				Component::Position3 position;
				Component::Transform transform;
				addComponent(position);
				addComponent(transform);
			}
		private:
			BRep brep_;
	};

};
