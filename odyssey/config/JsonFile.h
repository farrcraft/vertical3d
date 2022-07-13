/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <boost/json.hpp>

namespace odyssey::config {

    /**
     * This is based on the example JSON loading code in the boost library
     **/
    class JsonFile {
    public:
        /**
         **/
        JsonFile() = default;

        /**
         **/
        JsonFile(JsonFile&& other) noexcept;

        /**
         **/
        JsonFile(char const* path, char const* mode);

        /**
         **/
        JsonFile& operator=(JsonFile&& other) noexcept;


        /**
         **/
        ~JsonFile();

        /**
         **/
        void open(char const* path, char const* mode);

        /**
         **/
        void open(char const* path, char const* mode, boost::json::error_code& ec);

        /**
         **/
        long size() const noexcept;

        /**
         **/
        bool eof() const noexcept;

        /**
         **/
        void close();

        /**
         **/
        void fail(boost::json::error_code& ec);

        /**
         **/
        std::size_t read(char* data, std::size_t size, boost::json::error_code& ec);

        /**
         **/
        std::size_t read(char* data, std::size_t size);

    private:
        FILE* handle_ = nullptr;
        long size_ = 0;
    };


    inline std::string read_file(char const* path, boost::json::error_code& ec) {
        JsonFile f;
        f.open(path, "r", ec);
        if (ec) {
            return {};
        }
        std::string s;
        s.resize(f.size());
        s.resize(f.read(&s[0], s.size(), ec));
        if (ec) {
            return {};
        }
        return s;
    }

    inline std::string read_file(char const* path) {
        boost::json::error_code ec;
        auto s = read_file(path, ec);
        if (ec) {
            throw boost::json::system_error(ec);
        }
        return s;
    }

};
