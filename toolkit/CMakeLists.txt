set(EXPORT_BASENAME TOOLKIT)
set(PLATFORM_CONFIG_BASENAME TOOLKIT)
set(SYSTEM_CONFIG_BASENAME TOOLKIT)
set(VERSION_BASENAME TOOLKIT)

# Now no use BUILD_SHARED_LIBS 
# We need the same compile definitions between shared and static library 
# to make source files to compile once.
# if(BUILD_SHARED_LIBS) 
add_compile_definitions(${EXPORT_BASENAME}_SHARED_LIBS ${EXPORT_BASENAME}_SHARD_LIBS_EXPORTS)
# endif()


# for underground target no-rtti
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

# Specify include directories for both compilation and installation process.
# The $<INSTALL_PREFIX> generator expression is useful to ensure to create
# relocatable configuration files, see https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html#creating-relocatable-packages
# target_include_directories( ${LIBRARY_NAME}
	# INTERFACE	
	# "$<BUILD_INTERFACE:${LIBRARY_INCLUDE_PATH}>"
	# "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>"
	# "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
	# "$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}>"
	# )

PROJECT_INIT_TARGET_VARIABLE(toolkit)

add_library( ${TARGET_NAME} STATIC )

# require a dummy file to make sure the target is always built
set( DUMMY_FILE ${CMAKE_CURRENT_BINARY_DIR}/dummy.cc)
file(TOUCH ${DUMMY_FILE})

target_sources( ${TARGET_NAME}
	PRIVATE
	${DUMMY_FILE}
	)

# for upground target no-rtti
target_compile_options(${TARGET_NAME} INTERFACE -fno-rtti)

target_link_libraries( ${TARGET_NAME}
    PUBLIC
	util
    datadump
    )

add_subdirectory(util)
add_subdirectory(datadump)

######################################################################################
##  							INSTALL  											##
######################################################################################

PROJECT_INSTALL(${TARGET_NAME} ${TARGETS_EXPORT_NAME})
