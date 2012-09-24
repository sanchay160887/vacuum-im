add_definitions(-DHAVE_HUNSPELL) 
add_definitions(-DHUNSPELL_STATIC)

if (SYSTEM_HUNSPELL_FOUND)
	set(ADD_LIBS ${SYSTEM_HUNSPELL_FOUND})
	add_definitions(-DUSE_SYSTEM_HUNSPELL)
	message(STATUS "Spellchecker backend: system hunspell, found at ${SYSTEM_HUNSPELL_FOUND}")
else (SYSTEM_HUNSPELL_FOUND)
	set(ADD_LIBS hunspell)
	message(STATUS "Spellchecker backend: bundled hunspell")
endif (SYSTEM_HUNSPELL_FOUND)

set(SOURCES ${SOURCES} "hunspellchecker.cpp")
set(HEADERS ${HEADERS} "hunspellchecker.h")
