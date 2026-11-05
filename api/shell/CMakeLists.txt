v3d_add_api_library(shell
	"Shell.h" "Shell.cpp"
)

# PUBLIC: Shell.h takes the engine it serves and hands out the ui, the menu and the screen.
# The renderer is declared, not included, so no shell header names a Vulkan type.
target_link_libraries(v3dlib_shell PUBLIC v3dlib_engine v3dlib_event v3dlib_ui EnTT::EnTT)
# PRIVATE: start() reads the ui document through the manifest and its json asset, and present()
# hands the engine a capture through the app's window renderer.
target_link_libraries(v3dlib_shell PRIVATE v3dlib_asset v3dlib_render v3dlib_window)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
