set(CUR gubed)

# Define folder organization
set(APPS_FOLDER "Apps")

add_executable(${CUR}
	main.cpp
	foreigns.cpp
	foreigns.h
	instrumenter.cpp
	instrumenter.h
	linemapper.h
	vm.cpp
	vm.h
	ui.cpp
	ui.h
)

target_link_libraries(${CUR} PRIVATE
        wren
        utils
		conwin
)

# Set folder for this app target
set_target_properties(${CUR} PROPERTIES FOLDER ${APPS_FOLDER})
