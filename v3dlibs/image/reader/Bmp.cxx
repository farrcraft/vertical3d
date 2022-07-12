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

namespace v3d::image::reader {
    /**
     **/
    Bmp::Bmp(const boost::shared_ptr<v3d::core::Logger>& logger) : Reader(logger) {
    }

    /**
     **/
    boost::shared_ptr<Image> Bmp::read(std::string_view filename) {
        LOG_DEBUG(logger_) << "BMPReader::read - reading file: " << filename;

        std::fstream file;
        file.open(static_cast<std::string>(filename).c_str(), std::fstream::in | std::fstream::binary);

        boost::shared_ptr<Image> empty_ptr;

        if (!file) {
            LOG_DEBUG(logger_) << "BMPReader::read - error opening file: " << filename;
            return empty_ptr;
        }

        bmp_file_header fheader;
        memset(&fheader, 0, sizeof(bmp_file_header));

        // read file header
        file.read(reinterpret_cast<char*>(&fheader), sizeof(bmp_file_header));
        if (!file) {
            LOG_DEBUG(logger_) << "BMPReader::read - error reading bmp file header!";
            throw std::runtime_error("error reading bmp file header!");
        }

        // check magic number
        if (fheader.type_ != 19778) {
            LOG_DEBUG(logger_) << "BMPReader::read - bad header magic number!";
            return empty_ptr;
        }

        bmp_info_header iheader;
        memset(&iheader, 0, sizeof(bmp_info_header));

        // read info header
        file.read(reinterpret_cast<char*>(&iheader), sizeof(bmp_info_header));
        if (!file) {
            LOG_DEBUG(logger_) << "BMPReader::read - error reading bmp info header!";
            throw std::runtime_error("error reading bmp info header!");
        }

        int num_colors = 1 << iheader.bits_;

        LOG_DEBUG(logger_) << "BMPReader::read - bmp type: " << fheader.type_;
        LOG_DEBUG(logger_) << "BMPReader::read - bmp size: " << fheader.size_;
        LOG_DEBUG(logger_) << "BMPReader::read - bmp reserved1: " << fheader.reserved1_;
        LOG_DEBUG(logger_) << "BMPReader::read - bmp reserved2: " << fheader.reserved2_;
        LOG_DEBUG(logger_) << "BMPReader::read - bmp offset: " << fheader.offset_;
        LOG_DEBUG(logger_) << "BMPReader::read - info size: " << iheader.size_;
        LOG_DEBUG(logger_) << "BMPReader::read - info width: " << iheader.width_;
        LOG_DEBUG(logger_) << "BMPReader::read - info height: " << iheader.height_;
        LOG_DEBUG(logger_) << "BMPReader::read - info planes: " << iheader.planes_;
        LOG_DEBUG(logger_) << "BMPReader::read - info bits: " << iheader.bits_;
        LOG_DEBUG(logger_) << "BMPReader::read - info compression: " << iheader.compression_;
        LOG_DEBUG(logger_) << "BMPReader::read - info image size: " << iheader.imageSize_;
        LOG_DEBUG(logger_) << "BMPReader::read - info xppm: " << iheader.xppm_;
        LOG_DEBUG(logger_) << "BMPReader::read - info yppm: " << iheader.yppm_;
        LOG_DEBUG(logger_) << "BMPReader::read - info used: " << iheader.used_;
        LOG_DEBUG(logger_) << "BMPReader::read - info important: " << iheader.important_;
        LOG_DEBUG(logger_) << "BMPReader::read - num colors: " << num_colors;

        bmp_rgb_quad* colors = 0;
        if (iheader.bits_ == 8) {  // load 8 bit color palette
            colors = new bmp_rgb_quad[num_colors];
            file.read(reinterpret_cast<char*>(&colors), sizeof(bmp_rgb_quad) * num_colors);
        }

        if (!file) {
            throw std::runtime_error("error reading bmp colors!");
        }
        // size of image data buffer including boundary padding
        uint64_t size = iheader.width_ * iheader.height_ * (iheader.bits_ / 8);
        int64_t width, pad;
        width = pad = iheader.width_ * (iheader.bits_ / 8);
        // adjust pad width to dword boundary alignment
        while (pad % 4 != 0) {
            pad++;
        }

        LOG_DEBUG(logger_) << "BMPReader::read - allocating image bits: " << size;
        LOG_DEBUG(logger_) << "BMPReader::read - width is: " << width << " after padding: " << pad;

        // this is just temporary storage
        boost::shared_ptr<Image> img(new Image(size));
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
        file.read(reinterpret_cast<char*>(temp), size);

        LOG_DEBUG(logger_) << "BMPReader::read - done reading file..";

        if (!file) {
            throw std::runtime_error("error reading bmp data!");
        }

        // done reading in file
        file.close();

        auto offset = pad - width;
        uint64_t height = iheader.height_;

        boost::shared_ptr<Image> image(new Image(iheader.width_, height, 24));
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
            if (iheader.height_ > 0) {
                // count backwards so you start at the front of the image
                for (uint64_t i = 0; i < size; i += 3) {
                    // jump over the padding at the start of a new line
                    if ((i + 1) % pad == 0) {
                        i += offset;
                    }

                    // transfer the data
                    *(data + i + 2) = *(temp + i);
                    *(data + i + 1) = *(temp + i + 1);
                    *(data + i) = *(temp + i + 2);
                }
            } else {  // image parser for a forward image
                auto j = size - 3;
                // count backwards so you start at the front of the image
                // here you can start from the back of the file or the front,
                // after the header  The only problem is that some programs
                // will pad not only the data, but also the file size to
                // be divisible by 4 bytes.
                for (uint64_t i = 0; i < size; i += 3) {
                    // jump over the padding at the start of a new line
                    if ((i + 1) % pad == 0) {
                        i += offset;
                    }
                    // transfer the data
                    *(data + j + 2) = *(temp + i);
                    *(data + j + 1) = *(temp + i + 1);
                    *(data + j) = *(temp + i + 2);
                    j -= 3;
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
