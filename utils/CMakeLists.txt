set(CUR utils)

add_library(${CUR} STATIC 
	binder.h 
	singleton.h 
	strutils.cpp 
	strutils.h 
	types.h
	xstring.h
	cmdline.h
)
target_link_libraries(${CUR} PUBLIC wren)
target_include_directories(${CUR} PUBLIC ${CMAKE_SOURCE_DIR}/utils)

# Set folder for this library target
set_target_properties(${CUR} PROPERTIES FOLDER ${LIBRARIES_FOLDER})
