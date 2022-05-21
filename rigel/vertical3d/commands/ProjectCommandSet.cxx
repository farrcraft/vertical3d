/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "ProjectCommandSet.h"
#include "../Window.h"

#include <libv3dcommand/CommandDirectory.h>
#include <libv3dcore/Project.h>

#include <libxml++/parsers/domparser.h>

#include <sstream>
#include <iostream>

using namespace v3D;

std::string strtoi(int i)
{
	std::stringstream buf;
	buf << i;
	return buf.str();
}

ProjectCommandSet::ProjectCommandSet()
{
	CommandDirectory & dir = CommandDirectory::instance();
	dir.connect(this, "project::load");
	dir.connect(this, "project::save");
}

ProjectCommandSet::~ProjectCommandSet()
{
}

bool ProjectCommandSet::exec(const std::string & cmd)
{
	/*
		native i/o uses an xml storage backend.
		we use libxml++ for parsing which depends on glibmm.
		so we use a command outside of libv3dcore/Project to prevent 
		having that dependency there.
	*/
	if (cmd == "project::load")
	{
		std::string fileName = Window::instance()->fileChooser("Open Project", true);
		if (fileName != "")
			read(fileName);
	}
	else if (cmd == "project::save")
	{
		std::string fileName = Window::instance()->fileChooser("Save Project", false);
		if (fileName != "")
			write(fileName);
	}
	else
	{
		return false;
	}
	return true;
}

bool ProjectCommandSet::read(const std::string & fileName)
{
	Project::instance().clear();

	xmlpp::DomParser parser(fileName, false); // false = don't validate

	xmlpp::Document * doc = parser.get_document();
	xmlpp::Element * project_elem = doc->get_root_node();
	// get project name
	xmlpp::Attribute * name = project_elem->get_attribute("name");
	if (name)
		Project::instance().name(name->get_value());
	// get scene elements
	xmlpp::Node::NodeList scenes = project_elem->get_children("scene");
	xmlpp::Node::NodeList::iterator scene_it = scenes.begin();
	// for each scene
	for (; scene_it != scenes.end(); scene_it++)
	{
		xmlpp::Element * scene_elem = dynamic_cast<xmlpp::Element*>(*scene_it);
		// create scene and add it to the project
		ScenePtr scene = new Scene;
		Project::instance().scenes().push_back(scene);
		// get scene name
		name = scene_elem->get_attribute("name");
		if (name)
			scene->name(name->get_value());
		// get active status
		xmlpp::Attribute * sel = scene_elem->get_attribute("active");
		if (sel && (sel->get_value() == "true"))
		{
			Project::instance().activeScene(scene);
		}
		// get mesh elements
		xmlpp::Node::NodeList meshes = scene_elem->get_children("mesh");
		xmlpp::Node::NodeList::iterator mesh_it = meshes.begin();
		// for each mesh
		for (; mesh_it != meshes.end(); mesh_it++)
		{
			MeshPtr mesh = new Mesh;
			xmlpp::Element * mesh_elem = dynamic_cast<xmlpp::Element*>(*mesh_it);

			// get selected status
			sel = mesh_elem->get_attribute("selected");
			if (sel && (sel->get_value() == "true"))
			{
				mesh->selected(true);
				scene->select(mesh);
			}
			// get transform element
			xmlpp::Attribute * attr;
			xmlpp::Node::NodeList transforms = mesh_elem->get_children("transform");
			// assert(transform.size() == 1)
			if (transforms.size() > 0)
			{
				xmlpp::Element * transform = dynamic_cast<xmlpp::Element*>(transforms.front());
				// get translation element
				xmlpp::Node::NodeList trans = transform->get_children("translation");
				xmlpp::Element * trans_elem;
				if (trans.size() > 0)
				{
					trans_elem = dynamic_cast<xmlpp::Element*>(trans.front());
					Vector3 t;
					attr = trans_elem->get_attribute("x");
					if (attr)
						t[0] = atof(attr->get_value().c_str());
					attr = trans_elem->get_attribute("y");
					if (attr)
						t[1] = atof(attr->get_value().c_str());
					attr = trans_elem->get_attribute("z");
					if (attr)
						t[2] = atof(attr->get_value().c_str());
					mesh->translation(t);
				}
				// get rotation element
				trans = transform->get_children("rotation");
				if (trans.size() > 0)
				{
					trans_elem = dynamic_cast<xmlpp::Element*>(trans.front());
					Quaternion q;
					attr = trans_elem->get_attribute("x");
					if (attr)
						q[0] = atof(attr->get_value().c_str());
					attr = trans_elem->get_attribute("y");
					if (attr)
						q[1] = atof(attr->get_value().c_str());
					attr = trans_elem->get_attribute("z");
					if (attr)
						q[2] = atof(attr->get_value().c_str());
					attr = trans_elem->get_attribute("w");
					if (attr)
						q[3] = atof(attr->get_value().c_str());
					mesh->rotation(q);
				}
				// get scale element
				trans = transform->get_children("scale");
				if (trans.size() > 0)
				{
					trans_elem = dynamic_cast<xmlpp::Element*>(trans.front());
					Vector3 s;
					attr = trans_elem->get_attribute("x");
					if (attr)
						s[0] = atof(attr->get_value().c_str());
					attr = trans_elem->get_attribute("y");
					if (attr)
						s[1] = atof(attr->get_value().c_str());
					attr = trans_elem->get_attribute("z");
					if (attr)
						s[2] = atof(attr->get_value().c_str());
					mesh->scale(s);
				}
			}
			// get vertex elements
			xmlpp::Node::NodeList vertices = mesh_elem->get_children("vertex");
			xmlpp::Node::NodeList::iterator vertex_it = vertices.begin();
			// for each vertex
			for (; vertex_it != vertices.end(); vertex_it++)
			{
				xmlpp::Element * vertex_elem = dynamic_cast<xmlpp::Element*>(*vertex_it);
				Vector3 p;
				// get vertex attributes
				attr = vertex_elem->get_attribute("x");
				if (attr)
					p[0] = atof(attr->get_value().c_str());
				attr = vertex_elem->get_attribute("y");
				if (attr)
					p[1] = atof(attr->get_value().c_str());
				attr = vertex_elem->get_attribute("z");
				if (attr)
					p[2] = atof(attr->get_value().c_str());
				Vertex vert;
				vert.point(p);
				attr = vertex_elem->get_attribute("selected");
				if (attr && (attr->get_value() == "true"))
					vert.selected(true);
				mesh->addVertex(vert);
			}
			// get edge elements
			xmlpp::Node::NodeList edges = mesh_elem->get_children("edge");
			xmlpp::Node::NodeList::iterator edge_it = edges.begin();
			// for each edge element
			for (; edge_it != edges.end(); edge_it++)
			{
				xmlpp::Element * edge_elem = dynamic_cast<xmlpp::Element*>(*edge_it);
				// get edge attributes
				unsigned int v;
				attr = edge_elem->get_attribute("vertex");
				if (attr)
					v = atoi(attr->get_value().c_str());
				unsigned int f;
				attr = edge_elem->get_attribute("face");
				if (attr)
					f = atoi(attr->get_value().c_str());
				unsigned int e1, e2;
				attr = edge_elem->get_attribute("pair");
				if (attr)
					e1 = atoi(attr->get_value().c_str());
				attr = edge_elem->get_attribute("next");
				if (attr)
					e2 = atoi(attr->get_value().c_str());

				HalfEdge edge;
				edge.vertex(v);
				edge.next(e2);
				edge.face(f);
				edge.pair(e1);
				attr = edge_elem->get_attribute("selected");
				if (attr && (attr->get_value() == "true"))
					edge.selected(true);
				mesh->addEdge(edge);
			}
			// get face elements
			xmlpp::Node::NodeList faces = mesh_elem->get_children("face");
			xmlpp::Node::NodeList::iterator face_it = faces.begin();
			// for each face element
			for (; face_it != faces.end(); face_it++)
			{
				xmlpp::Element * face_elem = dynamic_cast<xmlpp::Element*>(*face_it);
				Face face;
				// get selected status
				attr = face_elem->get_attribute("selected");
				if (attr && (attr->get_value() == "true"))
					face.selected(true);

				unsigned int edgeID;
				attr = face_elem->get_attribute("edge");
				if (attr)
					edgeID = atoi(attr->get_value().c_str());
				face.edge(edgeID);
				mesh->addFace(face);
			}

			// add mesh to scene
			scene->meshes().push_back(mesh);
		}
	}
}
/*
<project name="project name">
	<scene name="scene name" active="true">
		<mesh selected="true">
			<transform>
				<translation x="1.0" y="1.0" z="1.0" />
				<rotation x="1.0" y="1.0" z="1.0" w="1.0" />
				<scale x="1.0" y="1.0" z="1.0" />
			</transform>
			<vertex x="1.0" y="2.0" z="1.0" selected="false" />
			<edge selected="true" vertex="vertexID" face="faceID" next="edgeID" pair="edgeID" />
			<face selected="false" edge="edgeID" />
			<!-- old way #2:
			<vertex x="1.0" y="2.0" z="1.0" selected="false" />
			<edge selected="true" prevVertex="vertexID" nextVertex="vertexID" prevFace="faceID" nextFace="faceID"
					prevCWEdge="edgeID" nextCWEdge="edgeID" prevCCWEdge="edgeID" nextCCWEdge="edgeID" />
			<face selected="false" edge="edgeID" />
			<!-- old way:
			<vertex x="1.0" y="2.0" z="1.0" selected="false" />
			<edge vertex1="0" vertex2="1" selected="true" />
			<polygon selected="false">
				<vertex index="0" />
			</polygon>
			-->
		</mesh>
	</scene>
</project>
*/

bool ProjectCommandSet::write(const std::string & fileName)
{
	xmlpp::Document * doc = new xmlpp::Document();
	xmlpp::Element * root = doc->create_root_node("project");
	root->set_attribute("name", Project::instance().name());
	std::vector<ScenePtr> & scenes = Project::instance().scenes();
	std::vector<ScenePtr>::iterator scene = scenes.begin();
	for (; scene != scenes.end(); scene++)
	{
		xmlpp::Element * scene_elem = root->add_child("scene");
		scene_elem->set_attribute("name", (*scene)->name());
		std::string val("false");
		if ((*scene)->active())
			val = "true";
		scene_elem->set_attribute("active", val);

		std::vector<MeshPtr> & meshes = (*scene)->meshes();
		std::vector<MeshPtr>::iterator mesh = meshes.begin();
		for (; mesh != meshes.end(); mesh++)
		{
			xmlpp::Element * mesh_elem = scene_elem->add_child("mesh");
			if ((*mesh)->selected())
				val = "true";
			else
				val = "false";
			mesh_elem->set_attribute("selected", val);

			// transform node
			xmlpp::Element * transform_elem = mesh_elem->add_child("transform");
			// translation node
			Vector3 translation = (*mesh)->translation();
			xmlpp::Element * translation_elem = transform_elem->add_child("translation");
			translation_elem->set_attribute("x", translation.str(0));
			translation_elem->set_attribute("y", translation.str(1));
			translation_elem->set_attribute("z", translation.str(2));
			// rotation node
			Quaternion rotation = (*mesh)->rotation();
			xmlpp::Element * rotation_elem = transform_elem->add_child("rotation");
			rotation_elem->set_attribute("x", rotation.str(0));
			rotation_elem->set_attribute("y", rotation.str(1));
			rotation_elem->set_attribute("z", rotation.str(2));
			rotation_elem->set_attribute("w", rotation.str(3));
			// scale node
			Vector3 scale = (*mesh)->scale();
			xmlpp::Element * scale_elem = transform_elem->add_child("scale");
			scale_elem->set_attribute("x", scale.str(0));
			scale_elem->set_attribute("y", scale.str(1));
			scale_elem->set_attribute("z", scale.str(2));
			// vertex nodes
			unsigned int vertex_count = 0;
			Mesh * brep = *mesh;
			for (; vertex_count < brep->vertexCount(); vertex_count++)
			{
				Vertex * vertex = brep->vertex(vertex_count);
				Vector3 point = vertex->point();

				xmlpp::Element * vertex_elem = mesh_elem->add_child("vertex");
				vertex_elem->set_attribute("x", point.str(0));
				vertex_elem->set_attribute("y", point.str(1));
				vertex_elem->set_attribute("z", point.str(2));
				if (vertex->selected())
					val = "true";
				else
					val = "false";
				vertex_elem->set_attribute("selected", val);
			}
			// edge nodes
			unsigned int edge_count = 0;
			for (; edge_count < brep->edgeCount(); edge_count++)
			{
				HalfEdge * edge = brep->edge(edge_count);
				xmlpp::Element * edge_elem = mesh_elem->add_child("edge");
				edge_elem->set_attribute("vertex", strtoi(edge->vertex()));
				edge_elem->set_attribute("pair", strtoi(edge->pair()));
				edge_elem->set_attribute("face", strtoi(edge->face()));
				edge_elem->set_attribute("next", strtoi(edge->next()));

				if (edge->selected())
					val = "true";
				else
					val = "false";
				edge_elem->set_attribute("selected", val);
			}
			// face nodes
			unsigned int face_count = 0;
			for (; face_count < brep->faceCount(); face_count++)
			{
				Face * face = brep->face(face_count);
				xmlpp::Element * face_elem = mesh_elem->add_child("face");
				face_elem->set_attribute("edge", strtoi(face->edge()));
				if (face->selected())
					val = "true";
				else
					val = "false";
				face_elem->set_attribute("selected", val);
			}
		}
	}
	doc->write_to_file(fileName);
	delete doc;
	return true;
}
