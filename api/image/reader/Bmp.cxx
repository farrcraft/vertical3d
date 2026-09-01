/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bmp.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
// needed for runtime_error
#include <stdexcept>

#include "../BmpHeader.h"

#include <boost/make_shared.hpp>

namespace v3d::image::reader {
    /**
     **/
    Bmp::Bmp(const boost::shared_ptr<v3d::log::Logger>& logger) : Reader(logger) {
    }

    /**
     **/
    boost::shared_ptr<Image> Bmp::read(std::string_view filename) {
        logger_->get()->debug("BMPReader::read - reading file: {}", filename);

        std::fstream file;
        file.open(static_cast<std::string>(filename).c_str(), std::fstream::in | std::fstream::binary);

        boost::shared_ptr<Image> empty_ptr;

        if (!file) {
            logger_->get()->error("BMPReader::read - error opening file: {}", filename);
            return empty_ptr;
        }

        bmp_file_header fheader;
        memset(&fheader, 0, sizeof(bmp_file_header));

        // read file header
        file.read(reinterpret_cast<char*>(&fheader), sizeof(bmp_file_header));
        if (!file) {
            logger_->get()->error("BMPReader::read - error reading bmp file header!");
            throw std::runtime_error("error reading bmp file header!");
        }

        // check magic number
        if (fheader.type_ != 19778) {
            logger_->get()->error("BMPReader::read - bad header magic number!");
            return empty_ptr;
        }

        bmp_info_header iheader;
        memset(&iheader, 0, sizeof(bmp_info_header));

        // read info header
        file.read(reinterpret_cast<char*>(&iheader), sizeof(bmp_info_header));
        if (!file) {
            logger_->get()->error("BMPReader::read - error reading bmp info header!");
            throw std::runtime_error("error reading bmp info header!");
        }

        int num_colors = 1 << iheader.bits_;

        logger_->get()->debug("BMPReader::read - bmp type: {}", fheader.type_);
        logger_->get()->debug("BMPReader::read - bmp size: {}", fheader.size_);
        logger_->get()->debug("BMPReader::read - bmp reserved1: {}", fheader.reserved1_);
        logger_->get()->debug("BMPReader::read - bmp reserved2: {}", fheader.reserved2_);
        logger_->get()->debug("BMPReader::read - bmp offset: {}", fheader.offset_);
        logger_->get()->debug("BMPReader::read - info size: {}", iheader.size_);
        logger_->get()->debug("BMPReader::read - info width: {}", iheader.width_);
        logger_->get()->debug("BMPReader::read - info height: {}", iheader.height_);
        logger_->get()->debug("BMPReader::read - info planes: {}", iheader.planes_);
        logger_->get()->debug("BMPReader::read - info bits: {}", iheader.bits_);
        logger_->get()->debug("BMPReader::read - info compression: {}", iheader.compression_);
        logger_->get()->debug("BMPReader::read - info image size: {}", iheader.imageSize_);
        logger_->get()->debug("BMPReader::read - info xppm: {}", iheader.xppm_);
        logger_->get()->debug("BMPReader::read - info yppm: {}", iheader.yppm_);
        logger_->get()->debug("BMPReader::read - info used: {}", iheader.used_);
        logger_->get()->debug("BMPReader::read - info important: {}", iheader.important_);
        logger_->get()->debug("BMPReader::read - num colors: {}", num_colors);

        bmp_rgb_quad* colors = 0;
        if (iheader.bits_ == 8) {  // load 8 bit color palette
            colors = new bmp_rgb_quad[num_colors];
            file.read(reinterpret_cast<char*>(&colors), sizeof(bmp_rgb_quad) * num_colors);
        }

        if (!file) {
            throw std::runtime_error("error reading bmp colors!");
        }
        int64_t width, pad;
        width = pad = iheader.width_ * (iheader.bits_ / 8);
        // adjust pad width to dword boundary alignment
        while (pad % 4 != 0) {
            pad++;
        }
        // a negative height means the rows are stored top down; either way there are that
        // many of them
        uint64_t rows = static_cast<uint64_t>(iheader.height_ < 0 ? -iheader.height_ : iheader.height_);
        // the image's own bytes, and the larger number of them the file actually holds -
        // every row on disk is padded out to a dword boundary
        uint64_t size = static_cast<uint64_t>(width) * rows;
        uint64_t storedSize = static_cast<uint64_t>(pad) * rows;

        logger_->get()->debug("BMPReader::read - allocating image bits: {}", size);
        logger_->get()->debug("BMPReader::read - width is: {} after padding: {}", width, pad);

        // this is just temporary storage
        boost::shared_ptr<Image> img = boost::make_shared<Image>(storedSize);
        unsigned char* temp = img->data();

        /*
        if (debug)
        {
            int pos = file.tellg();
            file.seekg(0, std::ios::end);
            int length = file.tellg();
            file.seekg(pos, std::ios::beg);
            std::cout << "file pointer: " << file.tellg() << std::endl;
            std::cout << "file length: " << length << std::endl;
        }
        */

        // read image data
        file.read(reinterpret_cast<char*>(temp), storedSize);

        logger_->get()->debug("BMPReader::read - done reading file..");

        if (!file) {
            throw std::runtime_error("error reading bmp data!");
        }

        // done reading in file
        file.close();

        auto offset = pad - width;

        boost::shared_ptr<Image> image(new Image(iheader.width_, static_cast<uint32_t>(rows), 24));
        unsigned char* data = image->data();

        // convert 8/24bit image from bgr to rgb
        if (iheader.bits_ == 8) {
            // diff = iheader._width * iheader._height * 3;
            // allocate the buffer for the final image data
            // data = new char[diff];

            if (iheader.height_ > 0) {
                int j = 0;
                // count backwards so you start at the front of the image
                for (uint64_t i = 0; i < size * 3; i += 3) {
                    // jump over the padding at the start of a new line
                    if ((i + 1) % pad == 0) {
                        i += offset;
                    }
                    // transfer the data
                    *(data + i) = colors[*(temp + j)].red_;
                    *(data + i + 1) = colors[*(temp + j)].green_;
                    *(data + i + 2) = colors[*(temp + j)].blue_;
                    j++;
                }
            } else {  // image parser for a forward image
                auto j = size - 1;
                // count backwards so you start at the front of the image
                for (uint64_t i = 0; i < size * 3; i += 3) {
                    // jump over the padding at the start of a new line
                    if ((i + 1) % pad == 0) {
                        i += offset;
                    }
                    // transfer the data
                    *(data + i) = colors[*(temp + j)].red_;
                    *(data + i + 1) = colors[*(temp + j)].green_;
                    *(data + i + 2) = colors[*(temp + j)].blue_;
                    j--;
                }
            }
            delete[] colors;
        } else if (iheader.bits_ == 16) {
            for (uint64_t i = 0; i < size; i += 2) {
                // jump over the padding at the start of a new line
                if ((i + 1) % pad == 0) {
                    i += offset;
                }
                // 0123 4567 0123 4567
                // 1234 5678 1234 5678
                // RRRR RGGG GGBB BBB0
                *(data + i) = ((*(temp + i) >> 3) << 3);  // R
                *(data + i + 1) = (*(temp + i) << 5) | ((*(temp + i + 1) >> 6) << 6);  // G
                *(data + i + 2) = ((*(temp + i + 1) >> 1) << 3);  // B
            }
        } else if (iheader.bits_ == 24) {
            // rows come off the disk padded to a dword boundary and go into the image
            // without that padding, so each is copied on its own. Walking both buffers with
            // one index and a modulo test for the padding read past the end of the data and
            // misplaced every row after the first - it only ever looked right because every
            // fixture is a single flat colour.
            for (uint64_t row = 0; row < rows; ++row) {
                const unsigned char* src = temp + row * pad;
                unsigned char* dest = data + row * width;
                for (int64_t column = 0; column < iheader.width_; ++column) {
                    // bgr on disk, rgb in memory
                    dest[column * 3 + 0] = src[column * 3 + 2];
                    dest[column * 3 + 1] = src[column * 3 + 1];
                    dest[column * 3 + 2] = src[column * 3 + 0];
                }
            }
        } else {
            std::stringstream ss;
            ss << iheader.bits_;
            throw std::runtime_error("unrecognized bmp bits - " + ss.str() + "!");
        }
        return image;
    }

};  // namespace v3d::image::reader
