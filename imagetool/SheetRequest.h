/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

/** What a --pack or --unpack run was asked to do. **/
class SheetRequest final {
 public:
    /** The images, and directories of images, to pack. **/
    std::vector<std::string> pack;
    /** The sprites document to unpack. **/
    std::string unpack;
    /** The sheet to pack, or the one sheet to unpack. Empty unpacks every sheet. **/
    std::string sheet;
    /** Where a pack writes the sheet's image. Empty writes <sheet>.png. **/
    std::string outfile;
    bool silent = false;
};
