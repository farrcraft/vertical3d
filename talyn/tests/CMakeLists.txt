# Talyn is an offline renderer and takes nothing from the realtime stack, so its suite needs
# neither a window nor a device.
v3d_add_test(talyn
	"TestMain.cxx"
	"RenderContextTest.cxx"
	"RIBHandlerTest.cxx"
	"ReferenceTest.cxx"
	"SceneTest.cxx")

# the suite reads its reference image through image::Factory, which is where the png and jpeg
# codecs come in
target_link_libraries(v3dtest_talyn PRIVATE v3dlib_talyn v3dlib_image ${Boost_LIBRARIES} ${PNG_LIBRARIES} JPEG::JPEG)

# The reader resolves a relative path against the working directory, which v3d_add_test
# points at the executable, so the fixtures have to sit beside it.
add_custom_command(TARGET v3dtest_talyn POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/data"
		"$<TARGET_FILE_DIR:v3dtest_talyn>/data"
	COMMENT "Copying talyn test fixtures"
	VERBATIM)
