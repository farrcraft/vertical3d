v3d_add_api_library(image
	"Image.h" "Image.cxx" "Compare.h" "Compare.cxx" "Crop.h" "Crop.cxx" "Factory.h" "Factory.cxx" "Reader.h" "Reader.cxx" "Writer.h" "Writer.cxx"
	"Texture.h" "Texture.cxx" "TextureAtlas.h" "TextureAtlas.cxx" "reader/Bmp.h" "reader/Bmp.cxx" "reader/Jpeg.h" "reader/Jpeg.cxx"
	"reader/Png.h" "reader/Png.cxx" "reader/Tga.h" "reader/Tga.cxx" "writer/Bmp.h" "writer/Bmp.cxx"
	"writer/Jpeg.h" "writer/Jpeg.cxx" "writer/Png.h" "writer/Png.cxx" "writer/Tga.h" "writer/Tga.cxx"
)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_image PUBLIC v3dlib_log)

# Factory resolves paths through boost::filesystem, so anything linking this library needs
# it too.
target_link_libraries(v3dlib_image PUBLIC Boost::filesystem)

# glm is named by Texture.h and TextureAtlas.h.
target_link_libraries(v3dlib_image PUBLIC glm::glm)

# PRIVATE: the codecs are reached only from reader/ and writer/, never from a header.
target_link_libraries(v3dlib_image PRIVATE PNG::PNG JPEG::JPEG)

target_compile_options(v3dlib_image PRIVATE /EHsc)

set(CMAKE_CXX_FLAGS "/utf-8")
set(CMAKE_C_FLAGS "/utf-8")

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
