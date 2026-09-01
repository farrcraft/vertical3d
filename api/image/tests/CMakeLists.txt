v3d_add_test(image
	"TestMain.cxx"
	"ImageTest.cxx"
	"ImageReaderTest.cxx"
	"ImageWriterTest.cxx"
	"TextureTest.cxx")

target_link_libraries(v3dtest_image PRIVATE v3dlib_image ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG)

# The readers resolve a relative path against the working directory, which v3d_add_test
# points at the executable, so the fixtures have to sit beside it.
add_custom_command(TARGET v3dtest_image POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_image>/data"
	COMMENT "Copying image test fixtures"
	VERBATIM)
