PROJECT_INIT_TARGET_VARIABLE(util)
 
add_library( ${TARGET_NAME} INTERFACE)

target_sources( ${TARGET_NAME}
	INTERFACE
	"$<BUILD_INTERFACE:${TARGET_HEADER_FILES}>"
	"$<BUILD_INTERFACE:${TARGET_SOURCE_FILES}>"
	)

target_include_directories( ${TARGET_NAME}
	INTERFACE	
	"$<BUILD_INTERFACE:${TARGET_INCLUDE_PATH}>"
	"$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>"
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
	)

target_link_libraries(${TARGET_NAME}
	INTERFACE
    framework::framework
)


######################################################################################
##  							INSTALL  											##
######################################################################################
install(FILES ${TARGET_HEADER_FILES}
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${TARGET_NAME}")

PROJECT_INSTALL(${TARGET_NAME} ${TARGETS_EXPORT_NAME})
