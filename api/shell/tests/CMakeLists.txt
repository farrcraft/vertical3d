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
v3d_copy_directory(v3dtest_shell fixtures "${CMAKE_CURRENT_SOURCE_DIR}/fixtures" "fixtures")

# The game theme the shell puts beneath a game's own, from the shared data every game copies.
v3d_copy_directory(v3dtest_shell theme "${V3D_ROOT}/data/ui" "fixtures/game/data/ui")
