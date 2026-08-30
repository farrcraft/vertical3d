/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Window3D.h"

// #include <GL/glew.h>

#include <sstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {
    Window3D::Window3D(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept :
        Window(logger), context_(nullptr), vulkanLoaded_(false), created_(false) {
    }

    /**
    **/
    bool Window3D::create(int width, int height) {
        // the loader has to be up before a window can be created with the vulkan flag
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            throw std::runtime_error(SDL_GetError());
        }
        vulkanLoaded_ = true;

        if (!Window::create(width, height, true)) {
            return false;
        }

        // the extensions SDL needs to be able to present to this window
        uint32_t extensionCount = 0;
        const char* const* extensionNames = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
        if (extensionNames == nullptr) {
            throw std::runtime_error("Failed to get Vulkan extensions from SDL: " + std::string(SDL_GetError()));
        }
        const std::vector<const char*> extensions(extensionNames, extensionNames + extensionCount);
/*
        context_ = SDL_GL_CreateContext(sdl());

        // need the experimental flag to get support for glGenVertexArrays
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            std::stringstream msg;
            msg << glewGetErrorString(err);
            throw std::runtime_error(msg.str());
        }

        // enable vsync
        SDL_GL_SetSwapInterval(1);

        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0f);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LEQUAL);
        glDepthRange(0.0f, 1.0f);
        glEnable(GL_DEPTH_CLAMP);
        // CCW winding is default
        glFrontFace(GL_CCW);
        glActiveTexture(GL_TEXTURE0);
*/
        instance_ = boost::make_shared<vulkan::Instance>(logger(), extensions);
        surface_ = boost::make_shared<vulkan::Surface>(instance_, sdl());

        created_ = true;
        return true;
    }

    /**
     **/
    void Window3D::destroy() {
        // the surface has to go before both the instance it belongs to and the window it presents to
        surface_.reset();
        instance_.reset();
        /*
        if (context_) {
            SDL_GL_DestroyContext(context_);
            context_ = nullptr;
        }
        */
        Window::destroy();
        if (vulkanLoaded_) {
            SDL_Vulkan_UnloadLibrary();
            vulkanLoaded_ = false;
        }
        created_ = false;
    }

    bool Window3D::created() const {
        return created_;
    }

    /**
     **/
    boost::shared_ptr<vulkan::Instance> Window3D::instance() const {
        return instance_;
    }

    /**
     **/
    boost::shared_ptr<vulkan::Surface> Window3D::surface() const {
        return surface_;
    }
};  // namespace v3d::render::realtime
