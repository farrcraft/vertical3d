# Make a directory hold what a source directory holds, as v3d_copy_directory's rule runs it:
#
#   cmake -DSOURCE=<dir> -DDESTINATION=<dir> -DMANIFEST=<file> -P v3dMirror.cmake
#
# Each file under SOURCE is copied to the same path under DESTINATION, unless the copy there
# already has the same contents. MANIFEST lists the files the last run copied, one path per line
# relative to SOURCE. A file it lists that SOURCE no longer holds is removed from DESTINATION. A
# file this copy never wrote is left alone, so two copies can share a destination, or one can
# sit inside the other's. The manifest is written last, once every file is in place.

foreach(variable SOURCE DESTINATION MANIFEST)
	if(NOT DEFINED ${variable})
		message(FATAL_ERROR "v3dMirror.cmake needs -D${variable}=")
	endif()
endforeach()
if(NOT IS_DIRECTORY "${SOURCE}")
	message(FATAL_ERROR "${SOURCE} is not a directory")
endif()

file(GLOB_RECURSE files LIST_DIRECTORIES false RELATIVE "${SOURCE}" "${SOURCE}/*")
list(SORT files)

set(copied "")
if(EXISTS "${MANIFEST}")
	file(STRINGS "${MANIFEST}" copied)
endif()
foreach(file IN LISTS copied)
	if(NOT file IN_LIST files)
		file(REMOVE "${DESTINATION}/${file}")
	endif()
endforeach()

foreach(file IN LISTS files)
	get_filename_component(directory "${DESTINATION}/${file}" DIRECTORY)
	file(MAKE_DIRECTORY "${directory}")
	file(COPY_FILE "${SOURCE}/${file}" "${DESTINATION}/${file}" ONLY_IF_DIFFERENT)
endforeach()

list(JOIN files "\n" listed)
file(WRITE "${MANIFEST}" "${listed}\n")
