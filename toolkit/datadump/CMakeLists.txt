PROJECT_INIT_TARGET_VARIABLE(datadump)

add_library( ${TARGET_NAME} STATIC)

target_sources( ${TARGET_NAME}
	PRIVATE
	"$<BUILD_INTERFACE:${TARGET_SOURCE_FILES}>"
	PUBLIC
	"$<BUILD_INTERFACE:${TARGET_HEADER_FILES}>"
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
	Eigen3::Eigen
)


######################################################################################
##  							INSTALL  											##
######################################################################################
install(FILES ${TARGET_HEADER_FILES}
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${TARGET_NAME}")

# Install threadsafe_queue headers
install(DIRECTORY threadsafe_queue/
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${TARGET_NAME}/threadsafe_queue"
	FILES_MATCHING PATTERN "*.h")

PROJECT_INSTALL(${TARGET_NAME} ${TARGETS_EXPORT_NAME})

######################################################################################
##  							TEST  											  ##
######################################################################################
if(BUILD_TEST)
	add_subdirectory(tests)
endif()


