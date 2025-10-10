cmake_minimum_required( VERSION 3.23.1 )

project(toolkit
        VERSION  1.0.0
		DESCRIPTION "Toolkit project for nrsdk!"
        LANGUAGES CXX C
        )


# Define relative dir for ${CMAKE_SOURCE_DIR}
set(PROJECT_SRC_DIR "toolkit")

######################################################################################
##  							INIT												##
######################################################################################
find_package(framework REQUIRED)
find_package(Eigen3 REQUIRED)


######################################################################################
##  							OPTIONS  											##
######################################################################################
# Log module variables


######################################################################################
##  							COMPILER											##
######################################################################################
# set(CMAKE_C_FLAGS " xxx")
# set(CMAKE_CXX_FLAGS " xxx")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(PROJECT_CMAKE_C_FLAGS "-Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-shorten-64-to-32")
	set(PROJECT_CMAKE_CXX_FLAGS "-Wno-deprecated -Wno-register -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-unused-private-field -Wno-shorten-64-to-32")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(PROJECT_CMAKE_C_FLAGS "-Wno-unused-variable -Wno-unused-parameter -Wno-unused-function")
	set(PROJECT_CMAKE_CXX_FLAGS "-Wno-register -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function -Wno-deprecated-declarations")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	set(PROJECT_CMAKE_CXX_FLAGS "/WX")
endif()

######################################################################################
##  							DOC													##
######################################################################################
# for doc generated
set(EXTERNAL_REQUIRE_DOC_DIR "${PROJECT_SOURCE_DIR}/${PROJECT_SRC_DIR}")

######################################################################################
##  							Config Project  									##
######################################################################################
set(INSTALL_EXPORT_CMAKE ON)
set(EXTERNAL_PROJECT_DIR ${PROJECT_SOURCE_DIR}/external/project)
list( APPEND CMAKE_MODULE_PATH "${EXTERNAL_PROJECT_DIR}/cmake" )

# After set environment variable, we includ project cmake.
include(Project)

######################################################################################
##  							GIT Hook											##
######################################################################################
# Set git hook
execute_process(COMMAND git config core.hooksPath external/project/githook)
