/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/


#include "HWRenderContext.h"

#include <windows.h>
#include <gl/gl.h>

#include "../v3dlibs/core/Scene.h"
#include "../v3dlibs/brep/BRep.h"

namespace v3d {

    void HWRenderContext::visit(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
        render(mesh);
    }

    void HWRenderContext::resize(int width, int height) {
        // map NDC coordinates to window coordinates
        glViewport(0, 0, width, height);
    }

    void HWRenderContext::prepare(const boost::shared_ptr<v3d::type::Camera>& camera) {
        // default lighting
        GLfloat ambient[] = { 0.5, 0.5, 0.5, 1.0 };
        GLfloat diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
        GLfloat position[] = { 2.0, 100.0, 2.0, 1.0 };
        glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
        glLightfv(GL_LIGHT1, GL_POSITION, position);
        glEnable(GL_LIGHT1);
        glEnable(GL_LIGHTING);
        glEnable(GL_COLOR_MATERIAL);

        // initialize projection matrix
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // append the camera's projection matrix
        camera->createProjection();
        Matrix4 p = camera->projection().transpose();
        glMultMatrixf(*p);

        // modelview contains modeling and viewing transforms
        glMatrixMode(GL_MODELVIEW);
        // start fresh
        glLoadIdentity();

        // since we use a right-handed coordinate space internally
        // we need to flip z when working with gl
        glScalef(1.0f, 1.0f, -1.0f);

        // apply camera transform
        // http://www.opengl.org/resources/faq/technical/viewing.htm
        // http://www.evl.uic.edu/ralph/508S98/coordinates.html
        camera->createView();
        Matrix4 m = camera->view().transpose();
        glMultMatrixf(*m);

        // set clear color to backgroundColor
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        glPolygonMode(GL_FRONT, frontRenderMode_);
        glPolygonMode(GL_BACK, backRenderMode_);
        if (cullMode_ != POLY_CULL_NONE) {
            glEnable(GL_CULL_FACE);
            glCullFace(cullMode_);
        }  else {
            glDisable(GL_CULL_FACE);
        }
    }

    // render an entire scene
    void HWRenderContext::render(const boost::shared_ptr<v3d::core::Scene>& scene) {
        scene->accept(this);
    }

    // render a single mesh
    void HWRenderContext::render(const boost::shared_ptr<v3d::brep::BRep>& mesh) {
        // set the draw color
        glColor3f(1.0f, 1.0f, 1.0f);

        // apply mesh transform
        glPushMatrix();
        // glMultMatrixf(mesh->matrix().transpose());

        // first pass - draw mesh normally
        glPolygonMode(GL_FRONT, GL_FILL);
        glPolygonMode(GL_BACK, GL_FILL);
        glPolygonOffset(1.0, 1.0);

        // draw mesh faces
        unsigned int face_count = 0;
        for (; face_count < mesh->faceCount(); face_count++) {
            glColor3f(0.1f, 0.1f, 0.5f);

            // set drawing mode for face - filled polygons or wireframes
            glBegin(GL_POLYGON);

            v3d::brep::BRep::vertex_iterator it(mesh, face_count);
            Vertex* vertex;
            for (; *it != 0; it++) {
                Vector3 v;
                vertex = *it;
                v = vertex->point();
                glVertex3f(v[0], v[1], v[2]);
            }

            // done drawing face
            glEnd();
        }
    }

};  // namespace v3d
