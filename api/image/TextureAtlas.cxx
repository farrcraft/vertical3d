/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "TextureAtlas.h"

#include <boost/shared_ptr.hpp>

#include "Image.h"
#include "Factory.h"

namespace v3d::image {
    TextureAtlas::TextureAtlas(unsigned int width, unsigned int height, unsigned int depth, const boost::shared_ptr<v3d::log::Logger>& logger) :
        width_(width),
        height_(height),
        depth_(depth),
        id_(0),
        used_(0),
        logger_(logger) {
        if (depth != 1 && depth != 3 && depth != 4) {
            std::string err = "Invalid texture atlas depth - must be 1, 3, or 4";
            LOG_ERROR(logger) << err;
            throw std::runtime_error(err);
        }

        // 1 pixel border
        nodes_.push_back(glm::ivec3(1, 1, width - 2));

        size_t size = width * height * depth;
        image_.reset(new Image(width, height, depth * 8));
    }

    void TextureAtlas::write(const std::string& filename) {
        Factory factory(logger_);
        factory.write(filename, image_);
    }

    TextureAtlas::~TextureAtlas() {
    }

    unsigned int TextureAtlas::width() const {
        return width_;
    }

    unsigned int TextureAtlas::height() const {
        return height_;
    }

    unsigned int TextureAtlas::depth() const {
        return depth_;
    }

    unsigned int TextureAtlas::id() const {
        return id_;
    }

    glm::ivec4 TextureAtlas::region(unsigned int width, unsigned int height) {
        glm::ivec3 node;
        int bestHeight = INT_MAX;
        int bestWidth = INT_MAX;
        int bestIndex = -1;
        glm::ivec4 region(0, 0, width, height);
        for (unsigned int i = 0; i < nodes_.size(); ++i) {
            int y = fit(i, width, height);
            if (y >= 0) {
                node = nodes_[i];
                if (((y + static_cast<int>(height)) < bestHeight) ||
                    (((y + static_cast<int>(height)) == bestHeight) && (node.z < bestWidth))) {
                    bestHeight = y + height;
                    bestIndex = i;
                    bestWidth = node.z;
                    region.x = node.x;
                    region.y = y;
                }
            }
        }

        if (bestIndex == -1) {
            region.x = -1;
            region.y = -1;
            region.z = 0;
            region.w = 0;
            return region;
        }

        node.x = region.x;
        node.y = region.y + height;
        node.z = width;
        nodes_.insert(nodes_.begin() + bestIndex, node);

        glm::ivec3 prev;
        for (unsigned int i = bestIndex + 1; i < nodes_.size(); ++i) {
            prev = nodes_[i - 1];

            if (nodes_[i].x < (prev.x + prev.z)) {
                int shrink = prev.x + prev.z - nodes_[i].x;
                nodes_[i].x += shrink;
                nodes_[i].z -= shrink;
                if (nodes_[i].z <= 0) {
                    nodes_.erase(nodes_.begin() + i);
                    --i;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        merge();
        used_ += width * height;
        return region;
    }

    /**
     **/
    int TextureAtlas::fit(unsigned int index, unsigned int width, unsigned int height) {
        glm::ivec3 node = nodes_[index];
        int x = node.x;
        if ((x + width) > (width_ - 1)) {
            return -1;
        }
        int y = node.y;
        int widthLeft = width;
        unsigned int i = index;
        while (widthLeft > 0) {
            node = nodes_[i];
            if (node.y > y) {
                y = node.y;
            }
            if ((y + height) > (height_ - 1)) {
                return -1;
            }
            widthLeft -= node.z;
            ++i;
        }
        return y;
    }

    void TextureAtlas::merge() {
        for (unsigned int i = 0; i < nodes_.size() - 1; ++i) {
            if (nodes_[i].y == nodes_[i + 1].y) {
                nodes_[i].z += nodes_[i + 1].z;
                nodes_.erase(nodes_.begin() + (i + 1));
                --i;
            }
        }
    }

    void TextureAtlas::region(unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned char* data, unsigned int stride) {
        assert(x > 0);
        assert(y > 0);
        assert(x < (width_ - 1));
        assert((x + width) <= (width_ - 1));
        assert(y < (height_ - 1));
        assert((y + height) <= (height_ - 1));

        size_t charsize = sizeof(char);
        unsigned char* imgData = image_->data();
        for (unsigned int i = 0; i < height; ++i) {
            memcpy(imgData + ((y + i) * width_ + x) * charsize * depth_,
                data + (i * stride) * charsize, width * charsize * depth_);
        }
    }

    boost::shared_ptr<Image> TextureAtlas::image() {
        return image_;
    }

};  // namespace v3d::image
