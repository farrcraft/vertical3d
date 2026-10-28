v3d_add_api_library(model
	"Model.h" "Model.cxx"
)

# PUBLIC: Model.h holds a skeleton and its clips, and glm vertex values.
target_link_libraries(v3dlib_model PUBLIC v3dlib_animation glm::glm)
