/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <GL/glew.h>

#include <sstream>
#include <string>

#include "Scene.h"
#include "DebugOverlay.h"

#include "engine/Camera.h"
#include "engine/Light.h"
#include "engine/Material.h"
#include "voxel/MeshBuilder.h"
#include "voxel/ChunkBufferPool.h"
#include "engine/MaterialFactory.h"

#include "../../v3dlibs//gl/Shader.h"
#include "../../stark/AssetLoader.h"
#include "../../stark/ProgramFactory.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <boost/log/trivial.hpp>



void loadMaterials(boost::shared_ptr<v3d::gl::Program> program) {
    MaterialFactory factory(program);
    // BLOCK_TYPE_DIRT
    factory.create("materials[0]", glm::vec3(0.9f, 0.5f, 0.3f));
    // BLOCK_TYPE_GRASS
    factory.create("materials[1]", glm::vec3(0.13f, 0.56f, 0.19f));
    // BLOCK_TYPE_SAND
    factory.create("materials[2]", glm::vec3(0.9f, 0.88f, 0.58f));
    // BLOCK_TYPE_STONE
    factory.create("materials[3]", glm::vec3(0.75f, 0.75f, 0.75f));
    // BLOCK_TYPE_GRAVEL
    factory.create("materials[4]", glm::vec3(0.52f, 0.52f, 0.52f));
    // BLOCK_TYPE_WATER
    factory.create("materials[5]", glm::vec3(0.25f, 0.39f, 0.96f));
    // BLOCK_TYPE_ORE
    factory.create("materials[6]", glm::vec3(0.16f, 0.16f, 0.16f));
    // BLOCK_TYPE_WOOD
    factory.create("materials[7]", glm::vec3(0.5f, 0.29f, 0.02f));
    // BLOCK_TYPE_LAVA
    factory.create("materials[8]", glm::vec3(1.0f, 0.47f, 0.12f));
    // BLOCK_TYPE_GLASS
    factory.create("materials[9]", glm::vec3(0.76f, 0.87f, 1.0f));
    // BLOCK_TYPE_BEDROCK
    factory.create("materials[10]", glm::vec3(0.29f, 0.29f, 0.29f));
    // BLOCK_TYPE_CLAY
    factory.create("materials[11]", glm::vec3(0.86f, 0.86f, 0.86f));
    // BLOCK_TYPE_ICE
    factory.create("materials[12]", glm::vec3(0.96f, 0.96f, 0.96f));
    // BLOCK_TYPE_SANDSTONE
    factory.create("materials[13]", glm::vec3(0.85f, 0.81f, 0.56f));
    // BLOCK_TYPE_OBSIDIAN
    factory.create("materials[14]", glm::vec3(0.0f, 0.0f, 0.0f));
    // BLOCK_TYPE_SNOW
    factory.create("materials[15]", glm::vec3(0.91f, 0.91f, 0.91f));
}

Renderer::Renderer(const boost::shared_ptr<Scene> & scene, const boost::shared_ptr<AssetLoader> & loader) :
    scene_(scene),
    debug_(false),
    builder_(scene->chunks()) {
    // log GL version info
    std::stringstream msg;
    const GLubyte * renderer = glGetString(GL_RENDERER);
    const GLubyte * vendor = glGetString(GL_VENDOR);
    const GLubyte * version = glGetString(GL_VERSION);
    const GLubyte * glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    msg << "Renderer: " << renderer << std::endl;
    msg << " Vendor: " << vendor << std::endl;
    msg << " GL Version: " << version << std::endl;
    msg << " GLSL: " << glslVersion << std::endl;
    BOOST_LOG_TRIVIAL(info) << msg.str();

    // setup shaders
    std::string shaderName;
    shaderName = "shaders/voxel_ads";
    ProgramFactory factory(loader);
    boost::shared_ptr<v3d::gl::Program> voxelProgram = factory.create(v3d::gl::Shader::SHADER_TYPE_VERTEX|v3d::gl::Shader::SHADER_TYPE_FRAGMENT, shaderName);

    // setup shader program uniforms
    voxelProgram->enable();

    // camera & model matrices
    glm::mat4 model(1.0f);
    unsigned int modelMatrix = voxelProgram->uniform("modelMatrix");
    glUniformMatrix4fv(modelMatrix, 1, GL_FALSE, glm::value_ptr(model));

    // lighting
    // glm::mat4 view = scene_->camera()->view();
    // glm::mat4 worldToCam = glm::transpose(glm::inverse(view));
    glm::vec4 pos(50.0f, 0.0f, 0.0f, 1.0f);
    // pos = worldToCam * pos;
    /*
    msg << " light position: " << glm::to_string(pos) << std::endl;
    LOG4CXX_INFO(logger, msg.str());
    */
    Light light(
        pos,  // position
        glm::vec3(0.4f, 0.4f, 0.4f),  // ambient
        glm::vec3(1.0f, 1.0f, 1.0f),  // diffuse
        glm::vec3(1.0f, 1.0f, 1.0f));  // specular
    light.program(voxelProgram);

    loadMaterials(voxelProgram);

    voxelProgram->disable();
    programs_["voxel"] = voxelProgram;

    pool_.reset(new ChunkBufferPool());

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
    glEnable(GL_DEPTH_CLAMP);
    // CCW winding is default
    glFrontFace(GL_CCW);
    glActiveTexture(GL_TEXTURE0);

    // setup the shader program for text rendering
    boost::shared_ptr<v3d::gl::Program> textProgram = factory.create(v3d::gl::Shader::SHADER_TYPE_VERTEX|v3d::gl::Shader::SHADER_TYPE_FRAGMENT, "shaders/text");
    programs_["text"] = textProgram;

    debugOverlay_.reset(new DebugOverlay(scene_, textProgram, loader));
}


void Renderer::draw(Hookah::Window * window) {
    glClearColor(0.4f, 0.6f, 0.9f, 1.0f);
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    boost::shared_ptr<v3d::gl::Program> program = programs_["voxel"];
    program->enable();

    // glVertexAttribDivisor(1, 0);
    // glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);

    if (scene_->camera()->dirty()) {
        unsigned int viewMatrix = program->uniform("viewMatrix");
        glm::mat4 view;
        view = scene_->camera()->view();
        glUniformMatrix4fv(viewMatrix, 1, GL_FALSE, glm::value_ptr(view));

        // world-to-camera matrix is the transpose inverse of the view matrix
        glm::mat4 worldToCam = glm::transpose(glm::inverse(view));
        unsigned int normalMatrix = program->uniform("normalMatrix");
        glUniformMatrix3fv(normalMatrix, 1, GL_FALSE, glm::value_ptr(worldToCam));
        /*
        glm::vec4 sunPos(0.0f, 500.0f, 0.0f, 1.0f);
        sunPos = worldToCam * sunPos;
        unsigned int lightPosition = program->uniform("Light.Position");
        glUniform4fv(lightPosition, 1, glm::value_ptr(sunPos));
        */
        scene_->camera()->dirty(false);
    }

    pool_->render();

    program->disable();

    // render text
    if (debug_) {
        debugOverlay_->render();
    }
}

void Renderer::tick(unsigned int delta) {
    // update chunks
    size_t maxChunkUpdates = 16;
    builder_.build(pool_, maxChunkUpdates);

    if (debug_) {
        debugOverlay_->update(delta);
    }
}

void Renderer::resize(int width, int height) {
    glViewport(0, 0, width, height);

    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    // update the aspect ratio of the camera & reload the projection matrix
    scene_->camera()->perspective(
        90.0f,  // x fov
        w / h,  // aspect
        0.1f,  // near
        1000.0f);  // far
    glm::mat4 projection;
    projection = scene_->camera()->projection();
    boost::shared_ptr<v3d::gl::Program> voxelProgram = programs_["voxel"];
    voxelProgram->enable();
    unsigned int projectionMatrix = voxelProgram->uniform("projectionMatrix");
    // location, count, transpose, data
    glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, glm::value_ptr(projection));
    voxelProgram->disable();

    debugOverlay_->resize(w, h);
}

void Renderer::debug(bool status) {
    debug_ = status;
    debugOverlay_->enable(status);
}
