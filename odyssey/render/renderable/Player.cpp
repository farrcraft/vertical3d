/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2021 Joshua Farr (josh@farrcraft.com)
 **/

#include "Player.h"

#include "../operation/BlitTexture.h"
#include "../Surface.h"
#include "../Texture.h"
#include "../../../api/asset/Image.h"
#include "../Engine.h"
#include "../../engine/Unit.h"

#include <boost/make_shared.hpp>

using namespace odyssey::render;
using namespace odyssey::render::renderable;

/**
 **/
Player::Player(boost::shared_ptr<Engine> renderer, boost::shared_ptr<odyssey::engine::Player> player) :
    Renderable(renderer),
    player_(player) {

    //boost::shared_ptr<odyssey::asset::Image> asset = assetManager->load("sample.png", odyssey::asset::Type::IMAGE_PNG);
    boost::shared_ptr<v3d::asset::Asset> asset = renderer_->assetManager()->loadTypeFromExt("sample.png");
    boost::shared_ptr<v3d::asset::Image> img = boost::static_pointer_cast<v3d::asset::Image>(asset);

    // create surface from image
    boost::shared_ptr<odyssey::render::Surface> surface = boost::make_shared<odyssey::render::Surface>(img->image());

    // create texture from surface
    boost::shared_ptr<odyssey::render::Texture> texture = boost::make_shared<odyssey::render::Texture>(renderer_->context(), surface->surface(), img->image()->width(), img->image()->height());

    // we need to scale our texture to our tile size proportions
    texture->resize(odyssey::engine::unit::tile_width, odyssey::engine::unit::tile_height);

    // put the texture into the cache
    renderer_->textureCache()->cache("sample.png", texture);
}

/**
 **/
void Player::draw(boost::shared_ptr<Frame> frame) {
    // fetch our source texture from the cache
    boost::shared_ptr<Texture> source = renderer_->textureCache()->fetch("sample.png");
    // create a new blit texture render operation
    boost::shared_ptr<operation::BlitTexture> operation = boost::make_shared<operation::BlitTexture>(source, renderer_->backBuffer());
    // push onto frame
    frame->addOperation(operation);
}
