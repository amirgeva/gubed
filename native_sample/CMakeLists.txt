set(CUR native_sample)

add_library(${CUR} SHARED 
	dmain.cpp
	sample.cpp
	sample.h
)
target_link_libraries(${CUR} PRIVATE
	utils
)

# Set folder for this library target
set_target_properties(${CUR} PROPERTIES FOLDER ${LIBRARIES_FOLDER})

