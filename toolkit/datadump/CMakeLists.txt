PROJECT_INIT_TARGET_VARIABLE(datadump)

add_library( ${TARGET_NAME} OBJECT )

target_sources( ${TARGET_NAME}
	PRIVATE
	"$<BUILD_INTERFACE:${TARGET_HEADER_FILES}>"
	"$<BUILD_INTERFACE:${TARGET_SOURCE_FILES}>"
	)

target_include_directories( ${TARGET_NAME}
	PUBLIC	
	"$<BUILD_INTERFACE:${TARGET_INCLUDE_PATH}>"
	"$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>"
	"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
	)

target_link_libraries(${TARGET_NAME}
	PUBLIC
    framework::framework
)


######################################################################################
##  							INSTALL  											##
######################################################################################
install(FILES ${TARGET_HEADER_FILES}
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${TARGET_NAME}")

PROJECT_INSTALL(${TARGET_NAME} ${TARGETS_EXPORT_NAME})
