v3d_add_test(image
	"TestMain.cxx"
	"CompareTest.cxx"
	"ImageTest.cxx"
	"ImageReaderTest.cxx"
	"ImageWriterTest.cxx"
	"TextureAtlasTest.cxx"
	"CropTest.cxx"
	"PackTest.cxx"
	"SvgReaderTest.cxx"
	"IconTest.cxx")

target_link_libraries(v3dtest_image PRIVATE v3dlib_image v3dlib_log ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG)

# Factory resolves a relative path against the working directory. v3d_add_test sets that to
# the executable's directory, so the fixtures are copied there.
add_custom_command(TARGET v3dtest_image POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_image>/data"
	COMMENT "Copying image test fixtures"
	VERBATIM)

# The fuzz targets for the readers this library decodes itself. The fixtures are their corpus,
# so each reader is given every fixture, whatever its format.
v3d_add_fuzzer(tga "fuzz/TgaFuzzer.cxx" LIBRARIES v3dlib_image v3dlib_log CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/data")
v3d_add_fuzzer(bmp "fuzz/BmpFuzzer.cxx" LIBRARIES v3dlib_image v3dlib_log CORPUS "${CMAKE_CURRENT_SOURCE_DIR}/data")
