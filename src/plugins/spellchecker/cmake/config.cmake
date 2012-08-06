if (APPLE)
	set(INSTALL_PREFIX "/Applications")
	set(INSTALL_APP_DIR "vacuum")
	set(INSTALL_LIB_DIR "Frameworks")
	set(INSTALL_RES_DIR "Resources")
elseif (UNIX)
	set(INSTALL_PREFIX "/usr/local")
	set(INSTALL_APP_DIR "vacuum")
	set(INSTALL_LIB_DIR "lib" CACHE STRING "Name of directory for shared libraries on target system")
	set(INSTALL_RES_DIR "share")
elseif (WIN32)
	set(INSTALL_PREFIX "C:")
	set(INSTALL_APP_DIR "vacuum")
	set(INSTALL_LIB_DIR ".")
	set(INSTALL_RES_DIR ".")
endif (APPLE)

if (CMAKE_INSTALL_PREFIX STREQUAL "")
	set(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}")
endif (CMAKE_INSTALL_PREFIX STREQUAL "")

# All paths given relative to ${CMAKE_INSTALL_PREFIX}
if (WIN32)
	set(INSTALL_BINS ".")
	set(INSTALL_LIBS "${INSTALL_LIB_DIR}")
	set(INSTALL_PLUGINS "${INSTALL_LIB_DIR}/plugins")
	set(INSTALL_RESOURCES "${INSTALL_RES_DIR}/resources")
	set(INSTALL_DOCUMENTS ".")
	set(INSTALL_TRANSLATIONS "${INSTALL_RES_DIR}/translations")
	set(INSTALL_INCLUDES "sdk")
elseif (APPLE)
	set(INSTALL_BINS "")
	set(INSTALL_LIBS "${INSTALL_APP_DIR}/Contents/${INSTALL_LIB_DIR}")
	set(INSTALL_PLUGINS "${INSTALL_APP_DIR}/Contents/PlugIns")
	set(INSTALL_RESOURCES "${INSTALL_APP_DIR}/Contents/${INSTALL_RES_DIR}")
	set(INSTALL_DOCUMENTS "${INSTALL_APP_DIR}/Contents/${INSTALL_RES_DIR}")
	set(INSTALL_TRANSLATIONS "${INSTALL_APP_DIR}/Contents/${INSTALL_RES_DIR}/translations")
	set(INSTALL_INCLUDES "${INSTALL_APP_DIR}/Contents/${INSTALL_RES_DIR}/include")
elseif (UNIX)
	set(INSTALL_BINS "bin")
	set(INSTALL_LIBS "${INSTALL_LIB_DIR}")
	set(INSTALL_PLUGINS "${INSTALL_LIB_DIR}/${INSTALL_APP_DIR}/plugins")
	set(INSTALL_RESOURCES "${INSTALL_RES_DIR}/${INSTALL_APP_DIR}/resources")
	set(INSTALL_DOCUMENTS "${INSTALL_RES_DIR}/doc/${INSTALL_APP_DIR}")
	set(INSTALL_TRANSLATIONS "${INSTALL_RES_DIR}/${INSTALL_APP_DIR}/translations")
	set(INSTALL_INCLUDES "include/${INSTALL_APP_DIR}")
endif (WIN32)
