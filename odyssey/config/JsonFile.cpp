/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#include"JsonFile.h"

#include <cstdio>
#include <string>

using namespace odyssey::config;

/**
 **/
JsonFile::JsonFile(JsonFile&& other) noexcept
    : handle_(other.handle_) {
    other.handle_ = nullptr;
}

/**
 **/
JsonFile::JsonFile(char const* path, char const* mode) {
    open(path, mode);
}

/**
 **/
JsonFile& JsonFile::operator=(JsonFile&& other) noexcept {
    close();
    handle_ = other.handle_;
    other.handle_ = nullptr;
    return *this;
}

/**
 **/
JsonFile::~JsonFile() {

	if (handle_) {
		std::fclose(handle_);
	}
}

/**
 **/
void JsonFile::open(char const* path, char const* mode) {
    boost::json::error_code ec;
    open(path, mode, ec);
    if (ec) {
        throw boost::json::system_error(ec);
    }
}

/**
 **/
long JsonFile::size() const noexcept {
    return size_;
}

/**
 **/
bool JsonFile::eof() const noexcept {
    return std::feof(handle_) != 0;
}

/**
 **/
void JsonFile::close() {
    if (handle_) {
        std::fclose(handle_);
        handle_ = nullptr;
        size_ = 0;
    }
}

/**
 **/
void JsonFile::fail(boost::json::error_code& ec) {
    ec.assign(errno, boost::json::generic_category());
}

/**
 **/
void JsonFile::open(char const* path, char const* mode, boost::json::error_code& ec) {
    close();
    errno_t err = fopen_s(&handle_, path, mode);
    if (err != 0) {
        return fail(ec);
    }
    if (std::fseek(handle_, 0, SEEK_END) != 0) {
        return fail(ec);
    }
    size_ = std::ftell(handle_);
    if (size_ == -1) {
        size_ = 0;
        return fail(ec);
    }
    if (std::fseek(handle_, 0, SEEK_SET) != 0) {
        return fail(ec);
    }
}

/**
 **/
std::size_t JsonFile::read(char* data, std::size_t size, boost::json::error_code& ec) {
    auto const nread = std::fread(data, 1, size, handle_);
    if (std::ferror(handle_)) {
        ec.assign(errno, boost::json::generic_category());
    }
    return nread;
}

/**
 **/
std::size_t JsonFile::read(char* data, std::size_t size) {
    boost::json::error_code ec;
    auto const nread = read(data, size, ec);
    if (ec) {
        throw boost::json::system_error(ec);
    }
    return nread;
}
