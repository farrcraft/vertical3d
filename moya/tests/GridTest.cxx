/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// MSVC reports code NanoVDB's headers cannot reach after it inlines them, which it does not
// count as a warning in an external header, so the warning is switched off around them.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
#include <nanovdb/NanoVDB.h>
#include <nanovdb/tools/CreateNanoGrid.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <openvdb/openvdb.h>

#include <string>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

namespace {

constexpr int kSide = 24;
constexpr float kMost = 3.0F;

/** The density of a voxel of the test grid: a ramp along x that peaks at kMost, zero past it. **/
float densityAt(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= kSide || y >= kSide || z >= kSide) {
        return 0.0F;
    }
    return kMost * static_cast<float>(x + 1) / static_cast<float>(kSide);
}

/**
 * A grid of densities over a cube of kSide voxels a side, named density. OpenVDB registers the
 * grid types a file is read by first.
 **/
openvdb::FloatGrid::Ptr ramp() {
    openvdb::initialize();
    openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create(0.0F);
    grid->setName("density");
    openvdb::FloatGrid::Accessor access = grid->getAccessor();
    for (int z = 0; z < kSide; ++z) {
        for (int y = 0; y < kSide; ++y) {
            for (int x = 0; x < kSide; ++x) {
                access.setValue(openvdb::Coord(x, y, z), densityAt(x, y, z));
            }
        }
    }
    return grid;
}

// a grid written to a file through OpenVDB and read back holds every value it was written with,
// inside the cube and beyond it
BOOST_AUTO_TEST_CASE(moya_grid_a_written_grid_reads_back_as_it_was_test) {
    const openvdb::FloatGrid::Ptr written = ramp();
    const std::string path = (boost::filesystem::temp_directory_path() / "v3d_moya_grid.vdb").string();
    {
        openvdb::io::File file(path);
        file.write({ written });
        file.close();
    }

    openvdb::io::File file(path);
    file.open();
    const openvdb::FloatGrid::Ptr read = openvdb::gridPtrCast<openvdb::FloatGrid>(file.readGrid("density"));
    file.close();
    boost::filesystem::remove(path);

    BOOST_TEST_REQUIRE(read != nullptr);
    const openvdb::FloatGrid::ConstAccessor access = read->getConstAccessor();
    for (int z = -1; z <= kSide; ++z) {
        for (int y = -1; y <= kSide; ++y) {
            for (int x = -1; x <= kSide; ++x) {
                BOOST_TEST(access.getValue(openvdb::Coord(x, y, z)) == densityAt(x, y, z));
            }
        }
    }
}

// a grid converted to NanoVDB holds the same values, and its root holds the most of them, which
// tracking takes as the grid's majorant
BOOST_AUTO_TEST_CASE(moya_grid_a_converted_grid_holds_the_same_values_and_its_most_test) {
    const openvdb::FloatGrid::Ptr grid = ramp();
    const nanovdb::GridHandle<nanovdb::HostBuffer> handle = nanovdb::tools::createNanoGrid(*grid);
    const nanovdb::NanoGrid<float> * nano = handle.grid<float>();

    BOOST_TEST_REQUIRE(nano != nullptr);
    BOOST_TEST(std::string(nano->gridName()) == "density");
    const nanovdb::NanoGrid<float>::AccessorType access = nano->getAccessor();
    for (int z = -1; z <= kSide; ++z) {
        for (int y = -1; y <= kSide; ++y) {
            for (int x = -1; x <= kSide; ++x) {
                BOOST_TEST(access.getValue(nanovdb::Coord(x, y, z)) == densityAt(x, y, z));
            }
        }
    }
    BOOST_TEST(nano->tree().root().maximum() == kMost);
}

};  // namespace
