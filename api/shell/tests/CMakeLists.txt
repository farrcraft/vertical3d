# The shell over an engine with no window and a screen with no device. Screen builds every
# part with no renderer, and draws into its canvas without uploading anything.
v3d_add_test(shell
	"TestMain.cpp"
	"ShellTest.cpp")

target_link_libraries(v3dtest_shell PRIVATE
	v3dlib_engine
	v3dlib_event
	v3dlib_shell
	v3dlib_ui)

# A fixture is an app path, not a data path: initialize() appends "data/" to it.
add_custom_command(TARGET v3dtest_shell POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
		"$<TARGET_FILE_DIR:v3dtest_shell>/fixtures"
	COMMENT "Copying shell test fixtures"
	VERBATIM)

# The game theme the shell puts beneath a game's own, from the shared data every game copies.
add_custom_command(TARGET v3dtest_shell POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${V3D_ROOT}/data/ui"
		"$<TARGET_FILE_DIR:v3dtest_shell>/fixtures/game/data/ui"
	COMMENT "Copying the game theme into the shell test fixture"
	VERBATIM)
