/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/json.hpp>

namespace v3d::asset {

/**
 * Writing documents to disk, the counterpart of readFile and the Json loader for reading.
 *
 * A caller decides what its document holds; this decides only how it reaches the disk.
 **/

/**
 * Render a document as text a person can read and diff: a scalar, a vector and a record of
 * those stay on one line, everything else is indented two spaces a level, and a double prints
 * as the float it was widened from. A double beyond the range of a float prints in full.
 *
 * JSON has no form for infinity or NaN, and this prints them as std::to_chars spells them.
 * writeDocument() refuses a document holding one.
 *
 * Named for the document rather than for what it does, because an unqualified serialize() on a
 * boost::json::value resolves to the one-line boost::json::serialize through ADL.
 *
 * @return the document, with no trailing newline
 **/
std::string serializeDocument(const boost::json::value& document);

/**
 * Replace the file at path with bytes, whole or not at all.
 *
 * The bytes go to a sibling temporary that is renamed onto the target, so the file already
 * there survives every failure but the rename. Nothing is flushed to the device, so this is
 * atomic against a crash and not against power loss.
 *
 * @param path the file to replace, which need not exist
 * @param bytes what it should hold
 * @return whether path now holds bytes; on false it is untouched and no temporary is left
 **/
bool writeFile(const boost::filesystem::path& path, const std::string& bytes);

/**
 * Write a document through serializeDocument() and writeFile(), terminated with a newline.
 *
 * @return false for a document holding an infinite or NaN number, which leaves path untouched,
 *         and otherwise what writeFile() returns
 **/
bool writeDocument(const boost::filesystem::path& path, const boost::json::value& document);

};  // namespace v3d::asset
