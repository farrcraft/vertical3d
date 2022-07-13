/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "AssetLoader.h"

#include <fstream>


AssetLoader::AssetLoader(const std::string & path) :
    path_(path) {
}

std::string AssetLoader::path() const {
    return path_;
}

std::string AssetLoader::load(const std::string & filename) {
    std::string fullPath = path_ + filename;

    // read shader file content
    std::ifstream file(fullPath.c_str(), std::ios::in | std::ios::binary);
    if (!file) {
        std::string err = std::string("error loading asset file: ") + fullPath + std::string(" - ") + strerror(errno);
        throw std::runtime_error(err);
    }
    std::string content;
    file.seekg(0, std::ios::end);
    content.resize(static_cast<unsigned int>(file.tellg()));
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    file.close();

    return content;
}
