/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Writer.h"

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <fstream>
#include <string>
#include <system_error>

#include <boost/filesystem/operations.hpp>
#include <boost/system/error_code.hpp>

namespace v3d::asset {

namespace {

void print(std::string* out, const boost::json::value& value, int depth);

/**
 **/
bool scalar(const boost::json::value& value) {
    return !value.is_object() && !value.is_array();
}

/**
 * A point, a vector or a quaternion - the one shape kept on a line of its own.
 **/
bool scalarArray(const boost::json::value& value) {
    if (!value.is_array()) {
        return false;
    }
    return std::ranges::all_of(value.as_array(), scalar);
}

/**
 * Whether a value is kept on one line: a scalar, a vector, or a record of those - an edge or
 * a face. A mesh broken a number to a line is unreadable and its diff is noise.
 **/
bool compact(const boost::json::value& value) {
    if (scalar(value) || scalarArray(value)) {
        return true;
    }
    if (!value.is_object()) {
        return false;
    }
    return std::ranges::all_of(value.as_object(), [](const auto& entry) {
        return scalar(entry.value()) || scalarArray(entry.value());
    });
}

/**
 **/
void indent(std::string* out, int depth) {
    out->append(static_cast<std::size_t>(depth) * 2, ' ');
}

/**
 * Write a number as a float.
 *
 * boost::json serializes a double as 0E0 rather than as 0 - a form that is valid, unreadable,
 * and impossible to hand edit. The narrowing is exact for anything a float was widened to
 * make, which is what every document this tree writes holds.
 **/
void printNumber(std::string* out, const boost::json::value& value) {
    if (!value.is_double()) {
        out->append(boost::json::serialize(value));
        return;
    }
    char buffer[32];
    const std::to_chars_result result =
        std::to_chars(buffer, buffer + sizeof(buffer), static_cast<float>(value.as_double()));
    if (result.ec != std::errc()) {
        out->append(boost::json::serialize(value));
        return;
    }
    out->append(buffer, static_cast<std::size_t>(result.ptr - buffer));
}

/**
 **/
void printArray(std::string* out, const boost::json::array& values, int depth) {
    if (values.empty()) {
        out->append("[]");
        return;
    }
    const bool oneLine = scalarArray(boost::json::value(values));
    out->append("[");
    for (std::size_t item = 0; item < values.size(); item++) {
        if (item > 0) {
            out->append(oneLine ? ", " : ",");
        }
        if (!oneLine) {
            out->append("\n");
            indent(out, depth + 1);
        }
        print(out, values[item], depth + 1);
    }
    if (!oneLine) {
        out->append("\n");
        indent(out, depth);
    }
    out->append("]");
}

/**
 **/
void printObject(std::string* out, const boost::json::object& entries, int depth) {
    if (entries.empty()) {
        out->append("{}");
        return;
    }
    const bool oneLine = compact(boost::json::value(entries));
    out->append("{");
    std::size_t item = 0;
    for (const auto& entry : entries) {
        if (item++ > 0) {
            out->append(",");
        }
        if (oneLine) {
            out->append(" ");
        } else {
            out->append("\n");
            indent(out, depth + 1);
        }
        out->append(boost::json::serialize(boost::json::value(entry.key())));
        out->append(": ");
        print(out, entry.value(), depth + 1);
    }
    if (oneLine) {
        out->append(" ");
    } else {
        out->append("\n");
        indent(out, depth);
    }
    out->append("}");
}

/**
 **/
void print(std::string* out, const boost::json::value& value, int depth) {
    if (value.is_object()) {
        printObject(out, value.as_object(), depth);
    } else if (value.is_array()) {
        printArray(out, value.as_array(), depth);
    } else {
        printNumber(out, value);
    }
}

/**
 * The sibling the bytes are written to before the rename. Named from the target rather than
 * uniquely, so one left behind by a killed process is overwritten by the next write to that
 * path instead of accumulating.
 **/
boost::filesystem::path temporaryFor(const boost::filesystem::path& path) {
    boost::filesystem::path temporary(path);
    temporary += ".tmp";
    return temporary;
}

};  // namespace

/**
 **/
std::string serializeDocument(const boost::json::value& document) {
    std::string text;
    print(&text, document, 0);
    return text;
}

/**
 **/
bool writeFile(const boost::filesystem::path& path, const std::string& bytes) {
    const boost::filesystem::path temporary = temporaryFor(path);
    boost::system::error_code error;
    {
        std::ofstream file(temporary.c_str(), std::ios::binary | std::ios::trunc);
        if (!file) {
            return false;
        }
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        file.close();
        if (!file) {
            boost::filesystem::remove(temporary, error);
            return false;
        }
    }
    // the target is replaced rather than removed first, so a reader between the two calls
    // sees one document or the other and never an absent one
    boost::filesystem::rename(temporary, path, error);
    if (error) {
        boost::system::error_code ignored;
        boost::filesystem::remove(temporary, ignored);
        return false;
    }
    return true;
}

/**
 **/
bool writeDocument(const boost::filesystem::path& path, const boost::json::value& document) {
    return writeFile(path, serializeDocument(document) + "\n");
}

};  // namespace v3d::asset
