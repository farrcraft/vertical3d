/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Tga.h"

#include <fstream>
#include <iostream>
#include <cstring>
#include <string>

#pragma pack(push, 1)

struct tga_header {
    unsigned char id_;
    unsigned char colormap_;  // 0=none, 1=palette
    unsigned char type_;  // 0=none, 1=indexed, 2=rgb, 3=grey, 4=rle

    uint16_t cmap_start_;
    uint16_t cmap_length_;
    unsigned char cmap_bits_;  // 15, 16, 24, 32

    uint16_t xorigin_;
    uint16_t yorigin_;
    uint16_t width_;
    uint16_t height_;
    unsigned char bpp_;  // 8, 16, 24, 32
    unsigned char descriptor_;
};

struct tga_footer {
    int64_t extension_offset_;
    int64_t dev_dir_offset_;
    char signature_[16];
    unsigned char reserved_;
    unsigned char terminator_;
};

#pragma pack(pop)

namespace v3d::image::writer {
    /**
     **/
    Tga::Tga(const boost::shared_ptr<v3d::log::Logger> & logger) : Writer(logger) {
    }

    /**
     **/
    bool Tga::write(std::string_view filename, const boost::shared_ptr<Image>& img) {
        std::fstream file;
        file.open(static_cast<std::string>(filename).c_str(), std::fstream::out | std::fstream::binary);
        if (file.fail()) {
            return false;
        }

        tga_header fheader;
        memset(&fheader, 0, sizeof(tga_header));

        fheader.width_ = img->width();
        fheader.height_ = img->height();
        fheader.bpp_ = img->bpp();
        fheader.type_ = 2;  // rgb

        file.write(reinterpret_cast<char*>(&fheader), sizeof(fheader));

        unsigned int bytespp = fheader.bpp_ / 8;
        unsigned int size = fheader.width_ * fheader.height_ * bytespp;
        boost::shared_ptr<Image> tmp_img(new Image(size));
        unsigned char* data = img->data();
        unsigned char* tmp_data = tmp_img->data();

        if (file.fail()) {
            file.close();
            return false;
        }

        for (unsigned int i = 0; i < static_cast<int>(size); i += bytespp) {  // Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
            tmp_data[i] = data[i + 2];
            tmp_data[i + 1] = data[i + 1];
            tmp_data[i + 2] = data[i];
        }

        file.write(reinterpret_cast<char*>(tmp_data), size);

        tga_footer footer;
        memset(&footer, 0, sizeof(tga_footer));

        // footer.signature_ = 'TRUEVISION-XFILE';
        strncpy(footer.signature_, "TRUEVISION-XFILE", 16);
        footer.reserved_ = '.';
        file.write(reinterpret_cast<char*>(&footer), sizeof(footer));

        file.close();
        return true;
    }

};  // namespace v3d::image::writer
