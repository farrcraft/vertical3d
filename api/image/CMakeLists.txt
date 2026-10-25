v3d_add_api_library(image
	"format/BmpFileHeader.h" "format/BmpInfoHeader.h" "format/BmpRgbQuad.h"
	"Image.h" "Image.cxx" "Channels.h" "Channels.cxx" "Compare.h" "Compare.cxx" "Crop.h" "Crop.cxx" "Factory.h" "Factory.cxx" "File.h" "File.cxx" "format/JpegError.h" "Reader.h" "Reader.cxx" "Writer.h" "Writer.cxx"
	"TextureAtlas.h" "TextureAtlas.cxx" "reader/Bmp.h" "reader/Bmp.cxx" "reader/Jpeg.h" "reader/Jpeg.cxx"
	"reader/Png.h" "reader/Png.cxx" "reader/Tga.h" "reader/Tga.cxx" "writer/Bmp.h" "writer/Bmp.cxx"
	"writer/Jpeg.h" "writer/Jpeg.cxx" "writer/Png.h" "writer/Png.cxx" "writer/Tga.h" "writer/Tga.cxx"
)

# PUBLIC: these sources compile Logger.h, and so do their consumers.
target_link_libraries(v3dlib_image PUBLIC v3dlib_log)

# PRIVATE: Factory resolves paths through boost::filesystem in its source only.
target_link_libraries(v3dlib_image PRIVATE Boost::filesystem)

# glm is named by TextureAtlas.h.
target_link_libraries(v3dlib_image PUBLIC glm::glm)

# PRIVATE: the codecs are reached only from reader/ and writer/. format/JpegError.h includes
# the jpeg header, and only the two jpeg sources include it, so no consumer needs it.
target_link_libraries(v3dlib_image PRIVATE PNG::PNG JPEG::JPEG)

if(V3D_BUILD_TESTS)
	add_subdirectory("tests")
endif()
