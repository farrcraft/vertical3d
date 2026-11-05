# Write the source file that holds the build's version. Run with cmake -P at build time, so a
# commit or an edit since the configure is still named.
#
# The file is written only when its text changes. A build that changed nothing then compiles
# nothing, and a new commit recompiles one file.
#
# @param OUTPUT the source file to write
# @param NUMBER the release's number, as major.minor.patch
# @param RELEASE whether this is a release build, whose version is the bare number
# @param GIT the git executable, or empty when there is none
# @param ROOT the root of the tree, where git is asked which commit is checked out

if(NOT NUMBER MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
	message(FATAL_ERROR "the version has to be major.minor.patch, and the root's project() gives \"${NUMBER}\"")
endif()
string(REPLACE "." ";" parts "${NUMBER}")
list(GET parts 0 major)
list(GET parts 1 minor)
list(GET parts 2 patch)

set(text "${NUMBER}")
if(NOT RELEASE)
	set(described "")
	if(GIT)
		execute_process(COMMAND "${GIT}" describe --tags --always --dirty
			WORKING_DIRECTORY "${ROOT}"
			OUTPUT_VARIABLE described
			OUTPUT_STRIP_TRAILING_WHITESPACE
			RESULT_VARIABLE failed
			ERROR_QUIET)
		if(failed)
			set(described "")
		endif()
	endif()
	# what git describe names goes in the build metadata, after a +
	if(described STREQUAL "")
		set(text "${NUMBER}-dev")
	else()
		set(text "${NUMBER}-dev+${described}")
	endif()
endif()

set(content "// Written by cmake/v3dVersion.cmake at build time. Edits are lost on the next build.

#include <api/version/Version.h>

#include <string_view>

namespace v3d::version {

Number number() noexcept {
    return { ${major}U, ${minor}U, ${patch}U };
}

std::string_view text() noexcept {
    return \"${text}\";
}

};  // namespace v3d::version
")

set(previous "")
if(EXISTS "${OUTPUT}")
	file(READ "${OUTPUT}" previous)
endif()
if(NOT previous STREQUAL content)
	file(WRITE "${OUTPUT}" "${content}")
endif()
