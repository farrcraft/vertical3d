# moya_bench times the new core's hierarchies over fixed scenes. Its numbers mean something only
# in a Release build. moya's suite runs one short pass in every build, which keeps it working.
add_executable(moya_bench "Bench.cxx" "Measured.h" "Rays.h" "Rays.cxx" "Scenes.h" "Scenes.cxx")

# It reads the CPU device's node and instance records, and the kernel's rays and hits, to count
# the hierarchies' bytes and the rays that met something. It logs what its renders report, and
# writes the textured room's image where the system keeps temporary files. It writes the fog
# room's grid there through OpenVDB, which libmoya links PRIVATE.
target_link_libraries(moya_bench PRIVATE v3dlib_log v3dlib_moya v3dlib_moya_cpu v3dlib_moya_kernel Boost::filesystem Boost::program_options OpenVDB::openvdb)

# It traces through the CPU device, which loads its libraries from beside the executable.
v3d_moya_cpu_libraries(moya_bench)

# The textured room reads its material against MaterialX's standard libraries, from beside the
# executable, as moya does.
v3d_moya_materialx_libraries(moya_bench)
