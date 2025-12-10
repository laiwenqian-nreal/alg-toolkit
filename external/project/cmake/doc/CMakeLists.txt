find_package(Doxygen
    OPTIONAL_COMPONENTS dot mscgen dia
    )

if(DOXYGEN_FOUND)
    message(STATUS "doxygen - generate doc             YES ")

	# common config
	# set(DOXYGEN_RECURSIVE YES)
	set(DOXYGEN_GENERATE_LATEX NO)

	# project requires
	set(DOXYGEN_EXTRACT_LOCAL_CLASSES NO)
	set(DOXYGEN_SHOW_USED_FILES NO)
	set(DOXYGEN_SHOW_FILES NO)
	set(DOXYGEN_SHOW_NAMESPACES NO)
	set(DOXYGEN_USE_MDFILE_AS_MAINPAGE main.md)

	# for NR_ENUM
	set(DOXYGEN_ENABLE_PREPROCESSING YES)
	set(DOXYGEN_MACRO_EXPANSION YES)
	set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
	set(DOXYGEN_EXPAND_AS_DEFINED NR_ENUM8 NR_ENUM16 NR_ENUM NR_ENUM64)
	set(DOXYGEN_PREDEFINED ${DOXYGEN_PREDEFINED} "NR_ENUM8(x)=enum x : int8_t" "NR_ENUM16(x)=enum x : int16_t" "NR_ENUM(x)=enum x : int32_t" "NR_ENUM64(x)=enum x : int64_t")

	# https://github.com/jothepro/doxygen-awesome-css
	# require doxygen 1.9.6 or later
	# css requires
	set(DOXYGEN_GENERATE_TREEVIEW YES)
	set(DOXYGEN_DISABLE_INDEX NO)
	set(DOXYGEN_FULL_SIDEBAR NO)
	set(DOXYGEN_HTML_EXTRA_STYLESHEET ${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css/doxygen-awesome.css ${CMAKE_CURRENT_SOURCE_DIR}/doxygen-awesome-css/doxygen-awesome-sidebar-only.css)
	set(DOXYGEN_HTML_COLORSTYLE LIGHT)

	set(DOXYGEN_STRIP_CODE_COMMENTS NO)

	if(NOT TARGET_DOC)
		set(TARGET_DOC doc)
	endif()
	message (STATUS "EXTERNAL_REQUIRE_DOC_DIR: ${EXTERNAL_REQUIRE_DOC_DIR}")
    doxygen_add_docs(
        ${TARGET_DOC}
		${EXTERNAL_REQUIRE_DOC_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}
		ALL
        COMMENT "Generate man pages"
        )

	install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/
            DESTINATION ${CMAKE_INSTALL_DOCDIR}/${TARGET_DOC}
            OPTIONAL)

else()
    message(STATUS "doxygen - generate doc             NO ")
endif()
