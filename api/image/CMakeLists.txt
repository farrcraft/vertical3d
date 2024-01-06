include_directories(SYSTEM
	${Boost_INCLUDE_DIRS}
	${Png_INCLUDE_DIRS}
	#"../../vendor/libpng-1.6.40"
	#"../../vendor/libjpeg-turbo-3.0.1"
)

add_library("v3dlib_image" 
	"Image.h" "Image.cxx" "Factory.h" "Factory.cxx" "Reader.h" "Reader.cxx" "Writer.h" "Writer.cxx"
	"Texture.h" "Texture.cxx" "TextureAtlas.h" "TextureAtlas.cxx" "reader/Bmp.h" "reader/Bmp.cxx" "reader/Jpeg.h" "reader/Jpeg.cxx"
	"reader/Png.h" "reader/Png.cxx" "reader/Tga.h" "reader/Tga.cxx" "writer/Bmp.h" "writer/Bmp.cxx"
	"writer/Jpeg.h" "writer/Jpeg.cxx" "writer/Png.h" "writer/Png.cxx" "writer/Tga.h" "writer/Tga.cxx"
)

# Workaround for a boost log ABI bug - https://github.com/microsoft/vcpkg/discussions/22762
target_compile_definitions(v3dlib_image PUBLIC BOOST_USE_WINAPI_VERSION=0x0A00)
target_compile_definitions(v3dlib_image PUBLIC _WIN32_WINNT=0x0A00)
target_compile_definitions(v3dlib_image PUBLIC WINVER=0x0A00)

