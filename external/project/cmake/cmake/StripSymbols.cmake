message ( " CMAKE_NM: ${CMAKE_NM} " )
message ( " CMAKE_STRIP: ${CMAKE_STRIP} " )

find_package (Python3 COMPONENTS Interpreter)

MACRO( PROJECT_STRIP_LINK_OPTIONS _NAME _NAME_ALL_SYMBOLS _EXTRACT_SYMBOLS_TYPE _IS_DYN_LIB_OR_BIN)
	message("PROJECT_STRIP_LINK_OPTIONS: ${_NAME} ${_NAME_ALL_SYMBOLS} ${_EXTRACT_SYMBOLS_TYPE} ${_IS_DYN_LIB_OR_BIN}")
	set(EXTRACT_SYMBOLS_TYPE ${_EXTRACT_SYMBOLS_TYPE})
	if(${_IS_DYN_LIB_OR_BIN})
		set(BASE_NAME lib${_NAME})
		set(BASE_NAME_ALL_SYMBOLS lib${_NAME_ALL_SYMBOLS})
		# DYN_LIB
		if (WIN32)
			set(EXTENSION .dll)
			set(STRIP_PARAMETER )
			set(COMPILE_TYPE mingw)
			if ( "symbol_type_nr_plugin" STREQUAL "${EXTRACT_SYMBOLS_TYPE}" )
				set(EXTRACT_SYMBOLS_TYPE "symbol_type_nr_plugin_ext")
			endif()
		elseif(APPLE)
			set(EXTENSION .dylib)
			set(STRIP_PARAMETER -x)
			set(COMPILE_TYPE xcode)
			if ( "symbol_type_nr_plugin" STREQUAL "${EXTRACT_SYMBOLS_TYPE}" )
				set(EXTRACT_SYMBOLS_TYPE "symbol_type_nr_plugin_ext")
			endif()
		else()  # android and linux which use gcc as compiler and linker
			set(EXTENSION .so)
			set(STRIP_PARAMETER --strip-debug --strip-unneeded)
			set(COMPILE_TYPE default)
		endif()
	else()
		# BIN
		if (WIN32)
			set(EXTENSION .exe)
			set(BASE_NAME ${_NAME})
			set(BASE_NAME_ALL_SYMBOLS ${_NAME_ALL_SYMBOLS})
			set(STRIP_PARAMETER )
			set(COMPILE_TYPE default)
		else()
			set(EXTENSION)
			set(BASE_NAME ${_NAME})
			set(BASE_NAME_ALL_SYMBOLS ${_NAME_ALL_SYMBOLS})
			set(STRIP_PARAMETER )
			set(COMPILE_TYPE default)
		endif()
	endif()
	if(IOS)
		set(FULL_NAME ${CMAKE_BUILD_TYPE}-iphoneos/${BASE_NAME}${EXTENSION})
		set(FULL_NAME_ALL_SYMBOLS ${CMAKE_BUILD_TYPE}-iphoneos/${BASE_NAME_ALL_SYMBOLS}${EXTENSION})
	else()
		set(FULL_NAME ${BASE_NAME}${EXTENSION})
		set(FULL_NAME_ALL_SYMBOLS ${BASE_NAME_ALL_SYMBOLS}${EXTENSION})
	endif()

	# symbol_type_null is used for no symbols
	if ( NOT ("symbol_type_null" STREQUAL "${EXTRACT_SYMBOLS_TYPE}" ))
		# current symbol_type is not null

		# symbol_type_nr_plugin is used fixed .def file, 
		# and no need to compile all_symbols library to extract valid symbols.
		if ( "symbol_type_nr_plugin" STREQUAL "${EXTRACT_SYMBOLS_TYPE}" )
			add_custom_command(OUTPUT ${_NAME}.def
				COMMAND echo "dummy" > ${FULL_NAME_ALL_SYMBOLS}
				COMMAND ${Python3_EXECUTABLE} ${EXTERNAL_PROJECT_DIR}/cmake/cmake/extract_symbols.py ${COMPILE_TYPE} ${EXTRACT_SYMBOLS_TYPE} "dummy" ${_NAME}.def
				)
			set_target_properties(${_NAME_ALL_SYMBOLS} PROPERTIES EXCLUDE_FROM_ALL TRUE)
		else()
			add_custom_command(OUTPUT ${_NAME}.def
				COMMAND ${CMAKE_NM} ${FULL_NAME_ALL_SYMBOLS} > ${_NAME}.all
				COMMAND ${Python3_EXECUTABLE} ${EXTERNAL_PROJECT_DIR}/cmake/cmake/extract_symbols.py ${COMPILE_TYPE} ${EXTRACT_SYMBOLS_TYPE} ${_NAME}.all ${_NAME}.def
				DEPENDS ${_NAME_ALL_SYMBOLS}
				)
		endif()

		target_sources( ${_NAME}
			PRIVATE
			${CMAKE_CURRENT_BINARY_DIR}/${_NAME}.def
			)

		if (WIN32)
			target_link_options(${_NAME}
				PRIVATE
				${CMAKE_CURRENT_BINARY_DIR}/${_NAME}.def
				)
		elseif(APPLE)
			target_link_options(${_NAME}
				PRIVATE
				-Wl,-exported_symbols_list,${CMAKE_CURRENT_BINARY_DIR}/${_NAME}.def
				)
		else()
			target_link_options(${_NAME}
				PRIVATE
				-Wl,--version-script=${CMAKE_CURRENT_BINARY_DIR}/${_NAME}.def
				)
		endif()
	endif()

	if (WIN32)
		target_link_options(${_NAME}
			PRIVATE
			--for-linker --pdb=${_NAME}.pdb
		)
		# when all symbols generate pdb, the nm will not extract symbols from FULL_NAME_ALL_SYMBOLS file.
		# target_link_options(${_NAME_ALL_SYMBOLS}
		# 	PRIVATE
		# 	--for-linker --pdb=${_NAME_ALL_SYMBOLS}.pdb
		# )
	endif()

	add_dependencies(${_NAME} ${_NAME_ALL_SYMBOLS})

	add_custom_command(TARGET ${_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E rename ${FULL_NAME_ALL_SYMBOLS} ${BASE_NAME_ALL_SYMBOLS}.bak
		COMMAND ${CMAKE_COMMAND} -E copy ${FULL_NAME} ${FULL_NAME_ALL_SYMBOLS}
		COMMAND ${CMAKE_STRIP} ${STRIP_PARAMETER} ${FULL_NAME}
		# In android llvm-addr2line -f -i -e libnr_api_all_symbols.so 0000000000343508
		# In windows cv2pdb64.exe libnr_api_all_symbols.so could generate pdb file
		)

ENDMACRO()
