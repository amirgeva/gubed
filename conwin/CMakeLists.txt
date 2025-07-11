set(CUR conwin)

add_library(${CUR} STATIC
        conwin.h
        conwin.cpp
        console.h
        conwindows.cpp
        concurses.cpp
)

target_link_libraries(${CUR} PUBLIC utils)
target_include_directories(${CUR} PUBLIC ${CMAKE_SOURCE_DIR}/conwin)

# Set folder for this library target
set_target_properties(${CUR} PROPERTIES FOLDER ${LIBRARIES_FOLDER})

if(NOT WIN32)
    # Find ncursesw (wide character version) instead of regular ncurses
    find_library(CURSES_LIBRARIES NAMES ncursesw)
    find_path(CURSES_INCLUDE_DIR NAMES ncurses.h PATH_SUFFIXES ncursesw)

    if(NOT CURSES_LIBRARIES OR NOT CURSES_INCLUDE_DIR)
        message(FATAL_ERROR "ncursesw library not found")
    endif()

    target_link_libraries(${CUR} PRIVATE ${CURSES_LIBRARIES})
    target_include_directories(${CUR} PRIVATE ${CURSES_INCLUDE_DIR})
    target_compile_definitions(${CUR} PRIVATE _XOPEN_SOURCE_EXTENDED)
endif()