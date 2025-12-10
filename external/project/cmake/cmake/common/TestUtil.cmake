MACRO( TEST_TARGET _target_name _target_sources _target_link_libraries)

	add_executable( ${_target_name} )

	target_sources( ${_target_name}
		PRIVATE
		${_target_sources}
		)

	target_include_directories( ${_target_name}
		PRIVATE
		${EXTERNAL_INCLUDE_DIR}
		${PROJECT_SOURCE_DIR}
		${PROJECT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
        )

	target_link_libraries( ${_target_name}
		PRIVATE
		${_target_link_libraries}
		)

	add_test(
		NAME TEST.${_target_name}
		COMMAND ${_target_name} ${TEST_RUNNER_PARAMS}
		)

ENDMACRO()

MACRO( TEST_TARGET_WITH_STRIP _target_name _target_sources _target_link_libraries)
	TEST_TARGET(${_target_name} ${_target_sources} ${_target_link_libraries} ${ARGN})
	TEST_TARGET(${_target_name}_all_symbols ${_target_sources} ${_target_link_libraries} ${ARGN})
	PROJECT_STRIP_LINK_OPTIONS(${_target_name} ${_target_name}_all_symbols "symbol_type_null" FALSE)
ENDMACRO()

MACRO( APP_TARGET _target_name _target_sources _target_link_libraries)
	add_executable(${_target_name})

	target_sources(${_target_name}
		PRIVATE
		${_target_sources}
		)

	target_include_directories( ${_target_name}
		PRIVATE
		${EXTERNAL_INCLUDE_DIR}
		${PROJECT_SOURCE_DIR}
		${PROJECT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
        )

	target_link_libraries( ${_target_name}
		PRIVATE
		${_target_link_libraries}
		${ARGN}
		)
ENDMACRO()

MACRO( APP_TARGET_WITH_STRIP _target_name _target_sources _target_link_libraries)
	APP_TARGET(${_target_name} ${_target_sources} ${_target_link_libraries} ${ARGN})
	APP_TARGET(${_target_name}_all_symbols ${_target_sources} ${_target_link_libraries} ${ARGN})
	PROJECT_STRIP_LINK_OPTIONS(${_target_name} ${_target_name}_all_symbols "symbol_type_null" FALSE)
ENDMACRO()

MACRO( APP_INSTALL_TARGET _target_name _target_sources _target_link_libraries)
	APP_TARGET(${_target_name} ${_target_sources} ${_target_link_libraries} ${ARGN})
	install(
		TARGETS ${_target_name}
		LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES DSTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
ENDMACRO()

MACRO( APP_INSTALL_TARGET_WITH_STRIP _target_name _target_sources _target_link_libraries)
	APP_TARGET_WITH_STRIP(${_target_name} ${_target_sources} ${_target_link_libraries} ${ARGN})
	install(
		TARGETS ${_target_name}
		LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES DSTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
ENDMACRO()

MACRO( APP_LIBRARY _target_name _target_sources _target_link_libraries)
	add_library( ${_target_name} SHARED )

	target_sources(${_target_name}
		PRIVATE
		${_target_sources}
		)

	target_include_directories( ${_target_name}
		PRIVATE
		${EXTERNAL_INCLUDE_DIR}
		${PROJECT_SOURCE_DIR}
		${PROJECT_BINARY_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_BINARY_DIR}
        )

	target_link_libraries( ${_target_name}
		PRIVATE
		${_target_link_libraries}
		${ARGN}
		)

ENDMACRO()

MACRO( APP_LIBRARY_WITH_STRIP _target_name _target_sources _target_link_libraries)
	APP_LIBRARY(${_target_name} ${_target_sources} ${_target_link_libraries} ${ARGN})
	APP_LIBRARY(${_target_name}_all_symbols ${_target_sources} ${_target_link_libraries} ${ARGN})
	PROJECT_STRIP_LINK_OPTIONS(${_target_name} ${_target_name}_all_symbols "symbol_type_null" TRUE)
ENDMACRO()

MACRO( APP_INSTALL_LIBRARY _target_name _target_sources _target_link_libraries)
	APP_LIBRARY(${_target_name} ${_target_sources} ${_target_link_libraries} ${ARGN})
	install(
		TARGETS ${_target_name}
		LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES DSTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
ENDMACRO()

MACRO( APP_INSTALL_LIBRARY_WITH_STRIP _target_name _target_sources _target_link_libraries)
	APP_LIBRARY_WITH_STRIP(${_target_name} ${_target_sources} ${_target_link_libraries} ${ARGN})
	install(
		TARGETS ${_target_name}
		LIBRARY  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		ARCHIVE  DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME  DESTINATION "${CMAKE_INSTALL_BINDIR}"
		INCLUDES DSTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
ENDMACRO()
