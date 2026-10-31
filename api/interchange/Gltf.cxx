/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Gltf.h"

#include <api/asset/File.h>
#include <api/interchange/Scene.h>
#include <api/interchange/internal/Reader.h>

#include <algorithm>
#include <cstdlib>
#include <optional>
#include <string>

// cgltf.h is a C header, and cpplint sorts it with the C system headers rather than with
// the third party ones. Its implementation half is compiled once, in CgltfImpl.cxx.
#include <cgltf.h>  // NOLINT(build/include_order)

namespace v3d::interchange {

namespace {

/**
 * cgltf's file reader. The document and every buffer it names are read through
 * asset::readFile, as every other file the api reads is. cgltf has already joined a
 * buffer's uri to the document's directory and decoded its percent-escapes.
 **/
cgltf_result readThroughAssets(const cgltf_memory_options* /* memory */, const cgltf_file_options* /* file */,
    const char* path, cgltf_size* size, void** data) {
    const std::optional<std::string> contents = v3d::asset::readFile(path);
    if (!contents) {
        return cgltf_result_file_not_found;
    }
    // cgltf frees what it is given through releaseRead()
    void* copy = std::malloc(contents->empty() ? 1 : contents->size());
    if (copy == nullptr) {
        return cgltf_result_out_of_memory;
    }
    std::copy(contents->begin(), contents->end(), static_cast<char*>(copy));
    *size = contents->size();
    *data = copy;
    return cgltf_result_success;
}

void releaseRead(const cgltf_memory_options* /* memory */, const cgltf_file_options* /* file */, void* data) {
    std::free(data);  // NOLINT(cppcoreguidelines-no-malloc) - cgltf hands back what readThroughAssets() allocated
}

};  // namespace

Imported readGltf(const std::string& path) {
    Imported imported;
    cgltf_options options{};
    options.file.read = readThroughAssets;
    options.file.release = releaseRead;
    cgltf_data* data = nullptr;
    if (cgltf_parse_file(&options, path.c_str(), &data) != cgltf_result_success) {
        imported.problem = "could not read " + path + " as glTF";
        return imported;
    }
    // a .gltf names its buffers and a .glb holds them. Either way, no accessor can be read
    // until they are loaded. cgltf reads an accessor without checking its bounds, and
    // cgltf_validate() checks every accessor against its buffer before any is read
    if (cgltf_load_buffers(&options, data, path.c_str()) != cgltf_result_success) {
        imported.problem = "could not load the buffers of " + path;
    } else if (cgltf_validate(data) != cgltf_result_success) {
        imported.problem = path + " is not valid glTF";
    } else {
        imported = internal::Reader(*data).read();
    }
    cgltf_free(data);
    return imported;
}

};  // namespace v3d::interchange
