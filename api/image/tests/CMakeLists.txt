v3d_add_test(image
	"TestMain.cxx"
	"CompareTest.cxx"
	"ImageTest.cxx"
	"ImageReaderTest.cxx"
	"ImageWriterTest.cxx"
	"TextureAtlasTest.cxx"
	"CropTest.cxx"
	"PackTest.cxx")

target_link_libraries(v3dtest_image PRIVATE v3dlib_image v3dlib_log ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG)

# Factory resolves a relative path against the working directory. v3d_add_test sets that to
# the executable's directory, so the fixtures are copied there.
add_custom_command(TARGET v3dtest_image POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_image>/data"
	COMMENT "Copying image test fixtures"
	VERBATIM)
