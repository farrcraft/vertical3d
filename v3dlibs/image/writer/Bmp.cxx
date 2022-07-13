/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Bmp.h"

#include <cstring>
#include <fstream>
#include <string>

#include "../BmpHeader.h"

namespace v3d::image::writer {
    /**
     **/
    Bmp::Bmp(const boost::shared_ptr<v3d::core::Logger>& logger) : Writer(logger) {
    }

    bool Bmp::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
        std::fstream file;
        file.open(static_cast<std::string>(filename).c_str(), std::fstream::out | std::fstream::binary);

        if (file.fail())
            return false;

        bmp_file_header fheader;
        memset(&fheader, 0, sizeof(bmp_file_header));

        fheader.type_ = 19778;
        fheader.offset_ = sizeof(bmp_file_header) + sizeof(bmp_info_header);
        fheader.size_ = sizeof(img->data()) + fheader.offset_;

        // write file header
        file.write(reinterpret_cast<char*>(&fheader), sizeof(bmp_file_header));

        bmp_info_header iheader;
        memset(&iheader, 0, sizeof(bmp_info_header));

        iheader.size_ = sizeof(bmp_info_header);
        iheader.width_ = img->width();
        iheader.height_ = img->height();
        iheader.bits_ = img->bpp();
        iheader.compression_ = 0;

        int32_t width, pad;
        width = pad = iheader.width_ * (iheader.bits_ / 8);
        // adjust pad width to dword boundary alignment
        while (pad % 4 != 0) {
            pad++;
        }

        unsigned int channels = img->bpp() / 8;
        iheader.imageSize_ = img->width() * img->height() * channels;

        // write info header
        file.write(reinterpret_cast<char*>(&iheader), sizeof(bmp_info_header));

        // size of image data buffer including boundary padding
        int offset = pad - width;
        uint64_t height = iheader.height_;
        uint64_t size = pad * height;
        boost::shared_ptr<Image> image(new Image(size));
        unsigned char* data = image->data();
        unsigned char* temp = img->data();

        unsigned int k = 0;
        for (unsigned int i = 0; i < size; i += channels) {
            // jump over the padding at the start of a new line
            if ((i + 1) % pad == 0) {
                i += offset;
            }
            // transfer the data
            *(data + i + 2) = *(temp + k);
            *(data + i + 1) = *(temp + k + 1);
            *(data + i) = *(temp + k + 2);

            if (img->format() == v3d::image::Image::Format::RGBA) {
                *(data + i + 3) = *(temp + k + 3);
            }
            k += channels;
        }

        // write image data
        file.write(reinterpret_cast<char*>(data), size);

        // done reading in file
        file.close();

        return true;
    }

};  // namespace v3d::image::writer
