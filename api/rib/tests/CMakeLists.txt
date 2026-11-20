# The RIB library runs on the CPU alone, so every part of it is testable without a window or a
# device.
v3d_add_test(rib
	"TestMain.cxx"
	"LexerTest.cxx"
	"DeclarationsTest.cxx"
	"ReaderTest.cxx"
	"WriterTest.cxx")

target_link_libraries(v3dtest_rib PRIVATE v3dlib_rib v3dlib_log glm::glm)

# The suite opens its fixtures by a path relative to the working directory. v3d_add_test
# sets the working directory to the executable's directory.
add_custom_command(TARGET v3dtest_rib POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_rib>/data"
	COMMENT "Copying RIB test fixtures"
	VERBATIM)

# The fuzz target for RIB. The fixtures are its corpus.
v3d_add_fuzzer(rib "fuzz/RibFuzzer.cxx" LIBRARIES v3dlib_rib v3dlib_log CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/data")
