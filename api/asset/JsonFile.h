/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <string>

#include <boost/json.hpp>

namespace v3d::asset {

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
        bool open(char const* path, char const* mode, const boost::json::error_code& ec);

        /**
         **/
        int64_t size() const noexcept;

        /**
         **/
        bool eof() const noexcept;

        /**
         **/
        void close();

        /**
         **/
        std::size_t read(char* data, std::size_t size, const boost::json::error_code& ec);

        /**
         **/
        std::size_t read(char* data, std::size_t size);

     private:
        FILE* handle_ = nullptr;
        int64_t size_ = 0;
    };


    inline std::string read_file(char const* path, const boost::json::error_code& ec) {
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

};  // namespace v3d::asset
