/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace v3d::asset {

/**
 * The whole of a file, as bytes.
 *
 * The caller decides what a missing or unreadable file means, so this only reports that it
 * happened: a loader turns it into no asset and a log line, and a reader of a document the
 * user chose turns it into a message.
 *
 * The file is sized before it is read, and a read that returns fewer bytes than that size is
 * a failure.
 *
 * @return the contents, or nothing when the file would not open, could not be sized or a read
 *         failed - an empty file is an empty string, not nothing
 **/
std::optional<std::string> readFile(std::string_view path);

};  // namespace v3d::asset
