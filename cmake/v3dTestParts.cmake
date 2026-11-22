# Check that a suite split into parts runs each of its cases once. Run with cmake -P as the
# ctest test <lib>_parts that v3d_add_test registers beside a suite's parts.
#
# The patterns match the units at the suite's top level: its cases, and its Boost.Test suites,
# each of which runs whole. Every such unit must be matched by exactly one part's patterns. A unit
# matched by none would never run, and one matched by two would run twice. A part none of whose
# patterns match a unit fails too, as Boost.Test refuses a run that matches nothing. One pattern
# of a part may match nothing, since a build can leave the cases it names out.
#
# @param EXECUTABLE the suite's Boost.Test executable
# @param PARTS each part as name=pattern,pattern, the parts apart at a bar. A pattern is a
#     unit's name with a * at its start, its end or both, as Boost.Test's --run_test reads it

execute_process(COMMAND "${EXECUTABLE}" --list_content
	RESULT_VARIABLE result
	OUTPUT_VARIABLE listed
	ERROR_VARIABLE listed)
if(NOT result EQUAL 0)
	message(FATAL_ERROR "${EXECUTABLE} --list_content exited ${result}:\n${listed}")
endif()

# Boost.Test lists one unit to a line, the units of a suite indented under it, and marks each
# unit that runs by default with a trailing *
string(REPLACE "\r" "" listed "${listed}")
string(REPLACE "\n" ";" lines "${listed}")
set(units "")
foreach(line IN LISTS lines)
	if(line STREQUAL "" OR line MATCHES "^[ \t]")
		continue()
	endif()
	string(REGEX REPLACE "\\*$" "" name "${line}")
	list(APPEND units "${name}")
endforeach()
if(units STREQUAL "")
	message(FATAL_ERROR "${EXECUTABLE} lists no cases")
endif()

# Each pattern as a regular expression, beside the part it belongs to and the units it matched
string(REPLACE "|" ";" parts "${PARTS}")
set(patterns "")
set(owners "")
set(expressions "")
foreach(part IN LISTS parts)
	string(REGEX REPLACE "=.*$" "" name "${part}")
	string(REGEX REPLACE "^[^=]*=" "" joined "${part}")
	string(REPLACE "," ";" named "${joined}")
	foreach(pattern IN LISTS named)
		if(NOT pattern MATCHES "^\\*?[A-Za-z0-9_]+\\*?$")
			message(FATAL_ERROR "part ${name}: \"${pattern}\" is not a name with a * at its start or end")
		endif()
		string(REPLACE "*" ".*" expression "${pattern}")
		list(APPEND patterns "${pattern}")
		list(APPEND owners "${name}")
		list(APPEND expressions "^${expression}$")
	endforeach()
endforeach()
list(LENGTH patterns count)
math(EXPR last "${count} - 1")
set(names "")
foreach(part IN LISTS parts)
	string(REGEX REPLACE "=.*$" "" name "${part}")
	if(NOT name MATCHES "^[A-Za-z0-9_]+$")
		message(FATAL_ERROR "\"${name}\" is not a part's name: letters, digits and underscores")
	endif()
	list(APPEND names "${name}")
	set(matched_${name} 0)
endforeach()

set(failures "")
foreach(unit IN LISTS units)
	set(matching "")
	foreach(i RANGE ${last})
		list(GET expressions ${i} expression)
		if(unit MATCHES "${expression}")
			list(GET owners ${i} owner)
			list(GET patterns ${i} pattern)
			list(APPEND matching "${owner} (${pattern})")
			math(EXPR matched_${owner} "${matched_${owner}} + 1")
		endif()
	endforeach()
	list(LENGTH matching found)
	if(found EQUAL 0)
		string(APPEND failures "\n  ${unit} is in no part")
	elseif(found GREATER 1)
		list(JOIN matching ", " both)
		string(APPEND failures "\n  ${unit} is in more than one part: ${both}")
	endif()
endforeach()
foreach(name IN LISTS names)
	if(matched_${name} EQUAL 0)
		string(APPEND failures "\n  part ${name} matches nothing")
	endif()
endforeach()

if(NOT failures STREQUAL "")
	message(FATAL_ERROR "the parts do not run each case once:${failures}")
endif()
list(LENGTH units total)
list(LENGTH parts split)
message(STATUS "${total} units, each in one of ${split} parts")
