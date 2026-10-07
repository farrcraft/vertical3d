v3d_add_api_library(asset
	"Asset.h" "Asset.cpp"
	"File.h" "File.cpp"
	"Json.h" "Json.cpp"
	"Loader.h" "Loader.cpp"
	"Manager.h" "Manager.cpp"
	"Migration.h" "Migration.cpp"
	"Type.h"
	"Writer.h" "Writer.cpp"
	"kind/Json.h" "kind/Json.cpp"
	"kind/Text.h" "kind/Text.cpp"
	"loader/Json.h" "loader/Json.cpp"
	"loader/Text.h" "loader/Text.cpp")

# PUBLIC: these sources compile Logger.h, and so do their consumers.
# The loaders for anything but a document are registered from api/asset/media and api/audio,
# so this library links neither them nor what they decode.
target_link_libraries(v3dlib_asset PUBLIC v3dlib_log)

# PUBLIC: Manager.h names a boost::filesystem::path, and Json.h a boost::json type.
target_link_libraries(v3dlib_asset PUBLIC Boost::filesystem Boost::json)

target_compile_options(v3dlib_asset PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
