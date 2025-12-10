############################################################################################################
# Util.cmake
############################################################################################################

############################################################################################################
# Remove str from a cmake string variable;
# _variable:    variable : cmake variable not string, but store a string.
# _str:         string: string to be removed from the cmake variable. 
macro(REMOVE_STRING_FROM_VARIABLE _variable _str)
	if(NOT ${${_variable}} EQUAL "")
		message(STATUS "REMOVE_STRING_FROM_VARIABLE remove ${_str} before: ${${_variable}}")
		string(REGEX REPLACE ${_str} " " remove_result ${${_variable}})
		string(STRIP ${remove_result} strip_result)
		set(${_variable} "${strip_result}")
		message(STATUS "REMOVE_STRING_FROM_VARIABLE remove ${_str} after: ${${_variable}}")
	endif()
endmacro(REMOVE_STRING_FROM_VARIABLE)
############################################################################################################
# Append str to a string property of a target.
# _target:      string: target name.
# _property:    name of target’s list property not string property. e.g: INTERFACE_COMPILE_OPTIONS
# _str:         string: string to be appended to the property
macro(APPEND_TARGET_LIST_PROPERTY _target _property _str)
	get_target_property(current_property ${_target} ${_property})
	if(NOT current_property) # property non-existent or empty
		set_target_properties(${_target} PROPERTIES ${_property} "${_str}")
	else()
		list(APPEND current_property ${_str})
		message("APPEND_TARGET_PROPERTY ${current_property}")
		set_target_properties(${_target} PROPERTIES ${_property} "${current_property}")
	endif()
endmacro(APPEND_TARGET_LIST_PROPERTY)
############################################################################################################
# Remove str from a string property of a target.
# _target:      string: target name.
# _property:    name of target’s list property not string property. e.g: INTERFACE_COMPILE_OPTIONS
# _str:         string: string to be removed from the property
macro(REMOVE_TARGET_LIST_PROPERTY _target _property _str)
	get_target_property(current_property ${_target} ${_property})
	if(current_property) # property non-existent or empty
		list(REMOVE_ITEM current_property ${_str})
		message("REMOVE_TARGET_LIST_PROPERTY ${current_property}")
		set_target_properties(${_target} PROPERTIES ${_property} "${current_property}")
	endif()
endmacro(REMOVE_TARGET_LIST_PROPERTY)
