# moya_albedo integrates the GGX albedo table libmoya compiles, which energy compensation reads.
# It runs at build time, so the table is not kept in the tree. libmoya's CMakeLists.txt holds the
# rule that runs it.
add_executable(moya_albedo "Albedo.cxx")

# PRIVATE: the tool draws and weighs reflections through the kernel's own GGX, so the table and
# the lobe it compensates agree, and it integrates the table's points on oneTBB's threads.
target_link_libraries(moya_albedo PRIVATE v3dlib_moya_kernel TBB::tbb)
if(MSVC)
	target_compile_options(moya_albedo PRIVATE /utf-8)
endif()
