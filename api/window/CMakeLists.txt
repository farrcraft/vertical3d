v3d_add_api_library(window
	"Window.h" "Window.cpp"
)

# PUBLIC: Window.h names SDL_Window and glm::ivec2, and takes the logger.
target_link_libraries(v3dlib_window PUBLIC v3dlib_log SDL3::SDL3 glm::glm)
