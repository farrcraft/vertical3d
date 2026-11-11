# What the CPU's libraries share with libmoya: the nodes and instances the CPU traverses, the
# traversal, the instruction sets and each library's table of entry points. Headers alone, which
# hold no instruction-specific type, so libmoya and each library compile them alike.
add_library("v3dlib_moya_cpu" INTERFACE
	"Geometry.h"
	"Instance.h"
	"Isa.h"
	"Module.h"
	"MotionNode.h"
	"Node.h"
	"Profiled.h"
	"ProfileParams.h"
	"Tally.h"
	"Traversal.h"
)
target_include_directories("v3dlib_moya_cpu" INTERFACE "${V3D_ROOT}")

# INTERFACE: the traversal and the table name kernel types, which name glm's.
target_link_libraries("v3dlib_moya_cpu" INTERFACE v3dlib_moya_kernel glm::glm)

# The CPU device loads a library of its own source for its instruction set, from the running
# executable's directory. Each links the shared headers alone, never libmoya, so nothing it
# compiles for its instruction set reaches libmoya or another library. Its symbols are hidden but
# for the table it exports, so none of its inline functions stands in for an executable's own.
foreach(isa "sse42" "avx2" "avx512")
	add_library("moya_cpu_${isa}" SHARED "Entry.cxx" "internal/Boxes.h" "internal/Nearest.h")
	set_target_properties("moya_cpu_${isa}" PROPERTIES PREFIX "" CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)
	string(TOUPPER "${isa}" upper)
	target_compile_definitions("moya_cpu_${isa}" PRIVATE "MOYA_CPU_${upper}")
	target_link_libraries("moya_cpu_${isa}" PRIVATE v3dlib_moya_cpu v3dlib_moya_kernel glm::glm)
endforeach()

# The x86-64 levels: SSE4.2 is v2, AVX2 is v3 and AVX-512 is v4. MSVC names each level's newest
# extension.
if(MSVC)
	target_compile_options("moya_cpu_sse42" PRIVATE /arch:SSE4.2 /utf-8)
	target_compile_options("moya_cpu_avx2" PRIVATE /arch:AVX2 /utf-8)
	target_compile_options("moya_cpu_avx512" PRIVATE /arch:AVX512 /utf-8)
else()
	target_compile_options("moya_cpu_sse42" PRIVATE -march=x86-64-v2)
	target_compile_options("moya_cpu_avx2" PRIVATE -march=x86-64-v3)
	target_compile_options("moya_cpu_avx512" PRIVATE -march=x86-64-v4)
endif()

# Builds the CPU's libraries before an executable that loads them, and copies them beside it. The
# copy is a target of its own that runs on every build, so a library that changed is copied even
# when the executable is not linked again.
#
# @param target the executable
function(v3d_moya_cpu_libraries target)
	set(copies "")
	foreach(isa "sse42" "avx2" "avx512")
		list(APPEND copies COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:moya_cpu_${isa}>" "$<TARGET_FILE_DIR:${target}>")
	endforeach()
	add_custom_target("${target}_cpu_libraries" ${copies} VERBATIM)
	foreach(isa "sse42" "avx2" "avx512")
		add_dependencies("${target}_cpu_libraries" "moya_cpu_${isa}")
	endforeach()
	add_dependencies(${target} "${target}_cpu_libraries")
endfunction()
