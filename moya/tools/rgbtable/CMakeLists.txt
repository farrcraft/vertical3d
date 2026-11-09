# moya_rgbtable fits the RGB-to-spectrum table libmoya compiles. It runs at build time, so the
# table is not kept in the tree. libmoya's CMakeLists.txt holds the rule that runs it.
add_executable(moya_rgbtable "Fit.h" "Table.h" "RgbTable.cxx")

# PRIVATE: the tool reads the kernel's observer, ACES white and matrices, so its fit and the
# kernel's lookup see the same colours, and it fits the table's points on oneTBB's threads.
target_link_libraries(moya_rgbtable PRIVATE v3dlib_moya_kernel TBB::tbb)
if(MSVC)
	target_compile_options(moya_rgbtable PRIVATE /utf-8)
endif()
