/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Player.h"

#include "../../../api/render/realtime/operation/2D/Blit2DTexture.h"
#include "../../../api/render/realtime/Surface.h"
#include "../../../api/render/realtime/2D/Texture2D.h"
#include "../../../api/asset/Image.h"
#include "../../engine/Unit.h"

#include <boost/make_shared.hpp>

namespace odyssey::render::renderable {

    /**
     **/
    Player::Player(boost::shared_ptr<v3d::render::realtime::Engine2D> renderer, boost::shared_ptr<odyssey::engine::Player> player) :
        Renderable(renderer),
        player_(player) {

        //boost::shared_ptr<odyssey::asset::Image> asset = assetManager->load("sample.png", odyssey::asset::Type::IMAGE_PNG);
        boost::shared_ptr<v3d::asset::Asset> asset = renderer_->assetManager()->loadTypeFromExt("sample.png");
        boost::shared_ptr<v3d::asset::Image> img = boost::static_pointer_cast<v3d::asset::Image>(asset);

        // create surface from image
        boost::shared_ptr<v3d::render::realtime::Surface> surface = boost::make_shared<v3d::render::realtime::Surface>(img->image());

        // create texture from surface
        boost::shared_ptr<v3d::render::realtime::Texture2D> texture = boost::make_shared<v3d::render::realtime::Texture2D>(renderer_->context(), surface->surface(), img->image()->width(), img->image()->height());

        // we need to scale our texture to our tile size proportions
        texture->resize(odyssey::engine::unit::tile_width, odyssey::engine::unit::tile_height);

        // put the texture into the cache
        renderer_->textureCache()->cache("sample.png", texture);
    }

    /**
     **/
    void Player::draw(boost::shared_ptr<v3d::render::realtime::Frame> frame) {
        // fetch our source texture from the cache
        boost::shared_ptr<v3d::render::realtime::Texture2D> source = renderer_->textureCache()->fetch("sample.png");
        // create a new blit texture render operation
        boost::shared_ptr<v3d::render::realtime::operation::Blit2DTexture> operation = boost::make_shared<v3d::render::realtime::operation::Blit2DTexture>(source, renderer_->backBuffer());
        // push onto frame
        frame->addOperation(operation);
    }

};  // namespace odyssey::render::renderable
