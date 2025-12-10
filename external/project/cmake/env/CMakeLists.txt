PROJECT_INIT_TARGET_VARIABLE( env )
PROJECT_CONFIGURE_FILE(export.h)
PROJECT_CONFIGURE_FILE(system_config.h)
PROJECT_CONFIGURE_FILE(platform_config.h)
PROJECT_CONFIGURE_FILE(version.h)
######################################################################################
##  							INSTALL  											##
######################################################################################
install(FILES ${TARGET_HEADER_FILES}
	DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/${TARGET_NAME}")
